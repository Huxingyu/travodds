// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "demo.h"
#include "dcps/condition/statuscondition.h"
#include "dcps/condition/waitset.h"
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
extern char** environ;

namespace datasharing_demo {
using namespace std::chrono_literals;
void require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
void check(ReturnCode_t result, const char* operation) {
    require(result == RETCODE_OK, std::string(operation) + ": return code " + std::to_string(result));
}
Peer::~Peer() { if (fd_ >= 0) ::close(fd_); }
void Peer::transfer(char& value, bool sending) {
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    pollfd item{fd_, short(sending ? POLLOUT : POLLIN), 0};
    int result;
    do {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        result = poll(&item, 1, int(std::max<int64_t>(0, ms)));
    } while (result < 0 && errno == EINTR && std::chrono::steady_clock::now() < deadline);
    require(result > 0, "Control channel timeout/error");
    ssize_t count;
    do { count = sending ? ::send(fd_, &value, 1, MSG_NOSIGNAL) : recv(fd_, &value, 1, 0); }
    while (count < 0 && errno == EINTR);
    require(count == 1, "Control peer exited or transfer failed");
}
void Peer::send(char value) { transfer(value, true); }
void Peer::expect(char value) {
    char received = 0;
    try { transfer(received, false); }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + " while waiting for phase " + value);
    }
    require(received == value, "Unexpected control phase");
}

Session::Session(const Specification& specification, const std::string& directory, const std::string& topicName)
    : type(specification.keyed), specification_(specification), directory_(directory) {
    try {
        DomainParticipantQos qos;
        // Discovery and fallback use UDP; legacy SHM transport cannot masquerade as DataSharing.
        qos.transport.removeTransport(TRANSPORT_SHM_QOS);
        participant = TheParticipantFactory->create_participant(146, qos, nullptr, STATUS_MASK_NONE);
        require(participant, "create_participant failed");
        check(participant->registe_type(type.get_typename(), &type), "registe_type");
        topic_ = participant->create_topic(topicName.c_str(), type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        require(topic_, "create_topic failed");
        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        require(publisher && subscriber, "create publisher/subscriber failed");
    } catch (...) { cleanup(); throw; }
}
Session::~Session() { cleanup(); }
void Session::cleanup() noexcept {
    if (!participant) return;
    try { close(); }
    catch (const std::exception& e) { std::cerr << "Cleanup failed: " << e.what() << '\n'; }
}
void Session::close() {
    if (!participant) return;
    check(participant->delete_contained_entities(), "delete_contained_entities");
    check(TheParticipantFactory->delete_participant(participant), "delete_participant");
    participant = nullptr; writer = nullptr; nativeReader = fallbackReader = nullptr;
}
void Session::makeWriter() {
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.data_sharing.shared_memory_directory = directory_;
    qos.data_sharing.max_samples = 16;
    qos.data_sharing.max_readers = 4;
    qos.data_sharing.max_segment_bytes = 1024 * 1024;
    qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    qos.history.depth = 4;
    qos.resource_limits.max_instances = 4;
    qos.writer_data_lifecycle.autodispose_unregistered_instances = false;
    writer = publisher->create_datawriter(topic_, qos, nullptr, STATUS_MASK_NONE);
    require(writer, "create_datawriter failed");
}
DataReader* Session::makeReader(bool sharing) {
    DataReaderQos qos;
    qos.data_sharing.kind = sharing ? ON_DATA_SHARING_QOS : OFF_DATA_SHARING_QOS;
    qos.data_sharing.shared_memory_directory = directory_;
    qos.data_sharing.max_segment_bytes = 1024 * 1024;
    qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    qos.history.depth = 4;
    auto* reader = subscriber->create_datareader(topic_, qos, nullptr, STATUS_MASK_NONE);
    require(reader, "create_datareader failed");
    return reader;
}
void Session::write(uint64_t key, uint64_t sequence, const InstanceHandle_t& handle, bool useLoan) {
    if (useLoan) {
        SampleLoan loan;
        check(writer->loan_sample(loan), "loan_sample");
        *static_cast<Sample*>(loan.data()) = sample(key, sequence);
        check(writer->write(loan, handle), "write Loan");
        require(!loan, "Successful write did not consume Loan");
    } else {
        auto value = sample(key, sequence);
        check(writer->write(&value, handle), "write sample");
    }
}
Session::Taken Session::take(DataReader* reader) {
    auto& condition = reader->get_statuscondition();
    check(condition.set_enabled_statuses(DATA_AVAILABLE_STATUS), "enable DATA_AVAILABLE");
    WaitSet wait;
    check(wait.attach_condition(condition), "attach condition");
    Taken result;
    try {
        ConditionSeq active;
        check(wait.wait(active, {8, 0}), "wait for sample");
        check(reader->take(result.values, result.infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), "take");
        check(wait.detach_condition(condition), "detach condition");
    } catch (...) { wait.detach_condition(condition); throw; }
    require(result.values.length() == 1 && result.infos.length() == 1, "Expected one sample");
    return result;
}
SampleInfo Session::consume(uint64_t key, uint64_t sequence, bool hold, bool valid, InstanceStateKind state) {
    SampleInfo nativeInfo;
    for (auto* reader : {nativeReader, fallbackReader}) {
        if (!reader) continue;
        auto data = take(reader);
        require(data.infos[0].valid_data == valid, "Unexpected valid_data");
        require(data.infos[0].instance_state == state, "Unexpected instance state");
        require(data.values.has_loan() == (valid && reader == nativeReader), "Wrong data path/ownership");
        if (valid) require(samePayload(data.values[0], key, sequence), "Sample content/order mismatch");
        if (type.has_key()) {
            auto expected = sample(key, sequence);
            InstanceHandle_t handle;
            require(type.MakeKey(&expected, handle, true) == 0, "MakeKey failed");
            require(data.infos[0].instance_handle == handle, "Instance handle mismatch");
            require(reader->lookup_instance(&expected) == handle, "Reader lookup failed");
            Sample snapshot{};
            check(reader->get_key_value(&snapshot, handle), "Reader get_key_value");
            require(snapshot.key == key, "Reader key snapshot mismatch");
        }
        if (reader == nativeReader) {
            nativeInfo = data.infos[0];
            if (valid) { ++received; ++nativeLoans; } else ++controls;
        }
        if (hold && reader == nativeReader) {
            require(!held.has_loan(), "Already holding a Loan");
            held = std::move(data.values); heldInfo = std::move(data.infos);
        } else check(reader->return_loan(data.values, data.infos), "return_loan");
    }
    return nativeInfo;
}
void Session::assertCounts(bool writerRole) {
    require(type.serializations == (writerRole && specification_.mixed ? specification_.samples : 0), "Unexpected payload serializer count");
    require(type.deserializations == (!writerRole && specification_.mixed ? specification_.samples : 0), "Unexpected payload deserializer count");
    if (!writerRole) require(received == specification_.samples && nativeLoans == received, "Wrong sample/Loan count");
}

namespace {
struct Directory {
    std::string path;
    Directory() {
        char pattern[] = "/dev/shm/travodds-demo-XXXXXX";
        char* result = mkdtemp(pattern);
        require(result, "Cannot create private /dev/shm directory"); path = result;
    }
    ~Directory() { if (!path.empty()) rmdir(path.c_str()); } // never recursively remove an unknown file
    void remove() {
        require(std::filesystem::is_empty(path), "Writer left a shared segment behind");
        require(std::filesystem::remove(path), "Cannot remove shared directory"); path.clear();
    }
};
struct Child {
    pid_t pid = -1;
    ~Child() {
        if (pid <= 0) return;
        pid_t reaped;
        do { reaped = waitpid(pid, nullptr, WNOHANG); } while (reaped < 0 && errno == EINTR);
        if (reaped == 0) {
            kill(pid, SIGKILL);
            do { reaped = waitpid(pid, nullptr, 0); } while (reaped < 0 && errno == EINTR);
        }
    }
    void finish() {
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        int status = 0;
        while (std::chrono::steady_clock::now() < deadline) {
            const auto result = waitpid(pid, &status, WNOHANG);
            if (result == pid) {
                pid = -1;
                require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "Subscriber did not exit successfully");
                return;
            }
            require(result == 0 || (result == -1 && errno == EINTR), "waitpid failed");
            std::this_thread::sleep_for(2ms);
        }
        throw std::runtime_error("Subscriber exit timeout");
    }
};
void report(const Specification& spec, const Session& app, bool writer, const std::string& directory) {
    std::cout << "DS_RESULT {\"example\":\"" << spec.name << "\",\"role\":\"" << (writer ? "writer" : "reader")
        << "\",\"pid\":" << getpid() << ",\"samples\":" << (writer ? spec.samples : app.received)
        << ",\"native_loans\":" << app.nativeLoans << ",\"controls\":" << app.controls
        << ",\"serializations\":" << app.type.serializations << ",\"deserializations\":" << app.type.deserializations
        << ",\"held_after_deletes\":" << (writer ? "false" : "true")
        << ",\"directory\":\"" << directory << "\"}" << std::endl;
}
}

int runExample(int argc, char** argv, const Specification& spec, const Scenario& writerScenario, const Scenario& readerScenario) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << "Run without arguments: finite " << spec.name << " publisher/subscriber demonstration.\n";
            return 0;
        }
        if (argc == 6 && std::string(argv[1]) == "--reader") {
            Peer peer(std::stoi(argv[2])); ::close(std::stoi(argv[3]));
            Session app(spec, argv[4], argv[5]);
            app.nativeReader = app.makeReader(true);
            peer.send('R');
            auto probe = app.take(app.nativeReader);
            require(probe.values.has_loan() && probe.infos[0].valid_data &&
                samePayload(probe.values[0], spec.keyed ? 11 : 0, 0), "Native readiness probe failed");
            check(app.nativeReader->return_loan(probe.values, probe.infos), "return probe");
            if (spec.mixed) app.fallbackReader = app.makeReader(false);
            app.type.blockDeserialization = !spec.mixed;
            peer.send('P'); peer.expect('S'); app.type.resetCounts(); peer.send('A');
            readerScenario(app, peer); app.assertCounts(false);
            peer.send('F'); peer.expect('D');
            require(app.held.has_loan() && samePayload(app.held[0], spec.keyed ? 11 : 0, 1), "Loan lost after Writer deletion");
            check(app.subscriber->delete_datareader(app.nativeReader), "delete native Reader");
            require(samePayload(app.held[0], spec.keyed ? 11 : 0, 1), "Loan lost after Reader deletion");
            app.held.unloan(); app.heldInfo.unloan();
            app.close(); report(spec, app, false, argv[4]); peer.send('E');
            return 0;
        }
        require(argc == 1, "Use no arguments, or --help");
        Directory directory;
        const std::string path = directory.path;
        Session app(spec, path, "DataSharing." + std::string(spec.name) + "." + std::to_string(getpid()));
        app.makeWriter();
        int sockets[2]; require(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "socketpair failed");
        Peer peer(sockets[0]);
        Child child;
        std::vector<std::string> arguments{"/proc/self/exe", "--reader", std::to_string(sockets[1]),
            std::to_string(sockets[0]), path, app.topic_->get_name()};
        std::vector<char*> args;
        for (auto& value : arguments) args.push_back(value.data());
        args.push_back(nullptr);
        pid_t spawnedPid = -1;
        const auto spawned = posix_spawn(&spawnedPid, "/proc/self/exe", nullptr, nullptr, args.data(), environ);
        if (spawned == 0) child.pid = spawnedPid;
        ::close(sockets[1]); require(spawned == 0, "posix_spawn failed");
        peer.expect('R');
        // A match is not proof of mapping. Before any OFF Reader exists, block
        // payload CDR and retry this still-owned Loan until native commit works.
        SampleLoan probe;
        check(app.writer->loan_sample(probe), "loan readiness probe");
        *static_cast<Sample*>(probe.data()) = sample(spec.keyed ? 11 : 0, 0);
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        ReturnCode_t result = RETCODE_ERROR;
        do {
            result = app.writer->write(probe, HANDLE_NIL);
            if (result == RETCODE_OK) break;
            require(result == RETCODE_ERROR && bool(probe), "Unexpected readiness write error");
            std::this_thread::sleep_for(5ms);
        } while (std::chrono::steady_clock::now() < deadline);
        check(result, "native readiness probe");
        require(!probe, "Readiness Loan not consumed");
        peer.expect('P');
        const auto matchDeadline = std::chrono::steady_clock::now() + 8s;
        InstanceHandleSeq matches;
        do {
            check(app.writer->get_matched_subscriptions(matches), "get matched subscriptions");
            if (matches.size() == (spec.mixed ? 2u : 1u)) break;
            std::this_thread::sleep_for(2ms);
        } while (std::chrono::steady_clock::now() < matchDeadline);
        require(matches.size() == (spec.mixed ? 2u : 1u), "Expected Readers did not match");
        app.type.blockSerialization = !spec.mixed;
        app.type.resetCounts(); peer.send('S'); peer.expect('A');
        writerScenario(app, peer); app.assertCounts(true); peer.expect('F');
        check(app.writer->wait_for_acknowledgments({3, 0}), "wait for acknowledgments");
        check(app.publisher->delete_datawriter(app.writer), "delete Writer");
        require(std::filesystem::is_empty(path), "Shared file survives Writer deletion");
        peer.send('D'); peer.expect('E'); child.finish();
        app.close(); directory.remove(); report(spec, app, true, path);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "DataSharing " << spec.name << " failed (pid=" << getpid() << "): " << e.what() << '\n';
        return 1;
    }
}
}
