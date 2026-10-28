// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#pragma once
#include "sample.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include <functional>
#include <string>

namespace datasharing_demo {
void require(bool value, const std::string& message);
void check(ReturnCode_t result, const char* operation);
struct Specification { const char* name; bool keyed; bool mixed; unsigned samples; };
class Peer {
public:
    explicit Peer(int fd) : fd_(fd) {}
    ~Peer();
    Peer(const Peer&) = delete;
    void send(char value);
    void expect(char value);
private:
    int fd_;
    void transfer(char&, bool sending);
};

class Session {
public:
    Session(const Specification&, const std::string& directory, const std::string& topicName);
    ~Session();
    Session(const Session&) = delete;
    void close();
    void makeWriter();
    DataReader* makeReader(bool sharing);
    void write(uint64_t key, uint64_t sequence, const InstanceHandle_t& handle = HANDLE_NIL,
        bool useLoan = true);
    SampleInfo consume(uint64_t key, uint64_t sequence, bool hold = false,
        bool valid = true, InstanceStateKind state = ALIVE_INSTANCE_STATE);
    void assertCounts(bool writerRole);
    SampleSupport type;
    DomainParticipant* participant = nullptr;
    Publisher* publisher = nullptr;
    Subscriber* subscriber = nullptr;
    DataWriter* writer = nullptr;
    DataReader* nativeReader = nullptr;
    DataReader* fallbackReader = nullptr;
    SequenceData<Sample> held;
    SampleInfoSeq heldInfo;
    unsigned nativeLoans = 0, received = 0, controls = 0;
private:
    struct Taken { SequenceData<Sample> values; SampleInfoSeq infos; };
    Taken take(DataReader*);
    void cleanup() noexcept;
    Specification specification_;
    std::string directory_;
    Topic* topic_ = nullptr;
    friend int runExample(int, char**, const Specification&,
        const std::function<void(Session&, Peer&)>&, const std::function<void(Session&, Peer&)>&);
};
using Scenario = std::function<void(Session&, Peer&)>;
// No arguments launches a fresh subscriber process using the same executable.
int runExample(int argc, char** argv, const Specification&, const Scenario& writer, const Scenario& reader);
}
