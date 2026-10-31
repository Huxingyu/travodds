// Independent-process validation of the experimental protocol, not a DDS backend.
#include "shared_pool.hpp"
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

using Core = travodds_experimental::Pool<4, 64, 2>;
using travodds_experimental::Result;
namespace {
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
void transfer(int fd, void* data, size_t size, bool sending) {
    auto* cursor = static_cast<unsigned char*>(data);
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (size) {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(end - std::chrono::steady_clock::now()).count();
        require(left > 0, "peer deadline");
        pollfd item{fd, short(sending ? POLLOUT : POLLIN), 0};
        const int ready = poll(&item, 1, int(left));
        if (ready < 0 && errno == EINTR) continue;
        require(ready > 0, "peer did not become ready");
        const auto count = sending ? send(fd, cursor, size, MSG_NOSIGNAL) : recv(fd, cursor, size, 0);
        if (count < 0 && errno == EINTR) continue;
        require(count > 0, "peer disconnected");
        cursor += count; size -= size_t(count);
    }
}
void marker(int fd, char value, bool sending) {
    const char expected = value;
    transfer(fd, &value, 1, sending);
    require(sending || value == expected, "unexpected peer phase");
}
struct Mapping {
    int fd = -1;
    Core::State* data = nullptr;
    std::string path;
    Mapping() = default;
    Mapping(const Mapping&) = delete;
    ~Mapping() {
        if (data) munmap(data, sizeof(Core::State));
        if (fd >= 0) close(fd);
        if (!path.empty()) unlink(path.c_str());
    }
    void map() {
        auto* address = mmap(nullptr, sizeof(Core::State), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        require(address != MAP_FAILED, "map segment"); data = static_cast<Core::State*>(address);
    }
};
struct Peer {
    pid_t pid = -1;
    int socket = -1;
    ~Peer() {
        if (socket >= 0) close(socket);
        if (pid > 0) { kill(pid, SIGKILL); while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {} }
    }
    int wait(int flags) {
        const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (std::chrono::steady_clock::now() < end) {
            int status = 0; const auto result = waitpid(pid, &status, flags | WNOHANG);
            if (result == pid) { if (WIFEXITED(status) || WIFSIGNALED(status)) pid = -1; return status; }
            require(result == 0 || errno == EINTR, "waitpid"); poll(nullptr, 0, 1);
        }
        throw std::runtime_error("child watchdog");
    }
};
void fill(unsigned char* bytes, uint64_t value) {
    uint64_t payload[8]; for (size_t i = 0; i < 8; ++i) payload[i] = value + i;
    memcpy(bytes, payload, sizeof(payload));
}
void check(const unsigned char* bytes, uint64_t value) {
    uint64_t payload[8]; memcpy(payload, bytes, sizeof(payload));
    for (size_t i = 0; i < 8; ++i) require(payload[i] == value + i, "loan payload overwritten");
}
int reader(const char* path, int socket, uintptr_t parent_address) {
    Mapping mapping; mapping.fd = open(path, O_RDWR | O_CLOEXEC); require(mapping.fd >= 0, "open named segment after exec");
    // Reserve the old virtual address where possible: sharing must work through
    // independently mapped offsets, rather than inherited process addresses.
    void* blocked = mmap(reinterpret_cast<void*>(parent_address), sizeof(Core::State), PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    mapping.map();
    require(reinterpret_cast<uintptr_t>(mapping.data) != parent_address, "independent mapping address");
    require(Core::compatible(mapping.data, sizeof(Core::State), {0x261760062, 0x20261029}), "session/layout check");
    Core pool(*mapping.data);
    uintptr_t address = reinterpret_cast<uintptr_t>(mapping.data);
    transfer(socket, &address, sizeof(address), true);
    marker(socket, 'P', false);
    Core::Sample sample; require(pool.take(0, sample) == Result::Ok, "reader takes committed sample");
    require(sample.sequence == 1 && sample.size == 64, "sample metadata"); check(sample.data, 100);
    marker(socket, 'L', true);
    require(raise(SIGSTOP) == 0, "stop application holding its loan");
    marker(socket, 'D', false); check(sample.data, 100);
    require(pool.release_reader(sample.token, 0) == Result::Ok, "reader returns original loan");
    marker(socket, 'D', true);
    if (blocked != MAP_FAILED) munmap(blocked, sizeof(Core::State));
    return 0;
}
void parent() {
    Mapping mapping; char pattern[] = "/dev/shm/travodds-lockfree-exec-XXXXXX";
    mapping.fd = mkstemp(pattern); require(mapping.fd >= 0, "create private segment"); mapping.path = pattern;
    require(fcntl(mapping.fd, F_SETFD, FD_CLOEXEC) == 0, "do not inherit segment fd across exec");
    require(ftruncate(mapping.fd, sizeof(Core::State)) == 0, "size private segment"); mapping.map();
    require(Core::initialize(mapping.data, sizeof(Core::State), {0x261760062, 0x20261029}), "initialize");
    Core pool(*mapping.data);
    int sockets[2]; require(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "socketpair");
    Peer peer; peer.socket = sockets[0]; peer.pid = fork(); require(peer.pid >= 0, "fork");
    if (peer.pid == 0) {
        close(sockets[0]); const auto fd = std::to_string(sockets[1]);
        const auto address = std::to_string(reinterpret_cast<uintptr_t>(mapping.data));
        execl("/proc/self/exe", "/proc/self/exe", "--reader", mapping.path.c_str(), fd.c_str(), address.c_str(), nullptr);
        _exit(127);
    }
    close(sockets[1]); uintptr_t child_address = 0; transfer(peer.socket, &child_address, sizeof(child_address), false);
    require(child_address != reinterpret_cast<uintptr_t>(mapping.data), "different mapping confirmed");
    // A live mapping must outlast its directory entry.
    require(unlink(mapping.path.c_str()) == 0, "unlink precise private segment"); mapping.path.clear();
    Core::Reservation first; require(pool.reserve(first) == Result::Ok, "reserve first sample"); fill(first.data, 100);
    require(pool.commit(first, 64, 1, 1) == Result::Ok, "publish first sample");
    require(pool.release_writer(first.token) == Result::Ok, "release first Writer owner"); marker(peer.socket, 'P', true);
    marker(peer.socket, 'L', false); const int stopped = peer.wait(WUNTRACED);
    require(WIFSTOPPED(stopped) && WSTOPSIG(stopped) == SIGSTOP, "reader is actually stopped");
    // This Reader keeps slot 0 pinned. It cannot hold a global metadata lock;
    // 10,000 publications to other slots still complete in the parent process.
    for (uint64_t i = 2; i <= 10001; ++i) {
        Core::Reservation next; require(pool.reserve(next) == Result::Ok, "paused Reader must not block other free slots");
        require(next.token.slot != first.token.slot, "pinned slot reused"); fill(next.data, i);
        require(pool.commit(next, 64, i, 2) == Result::Ok, "commit to Reader 1 while Reader 0 stopped");
        Core::Sample received;
        require(pool.take(1, received) == Result::Ok && received.sequence == i, "other Reader continues receiving in order");
        check(received.data, i);
        require(pool.release_reader(received.token, 1) == Result::Ok, "other Reader returns its loan");
        require(pool.release_writer(next.token) == Result::Ok, "release unrelated publication");
    }
    check(first.data, 100); require(kill(peer.pid, SIGCONT) == 0, "continue Reader"); marker(peer.socket, 'D', true);
    marker(peer.socket, 'D', false); const auto status = peer.wait(0);
    require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "exec Reader exit");
    std::array<Core::Reservation, Core::slot_count> reused{};
    bool found_original = false;
    for (auto& reservation : reused) {
        require(pool.reserve(reservation) == Result::Ok, "reuse after return");
        if (reservation.token.slot == first.token.slot) {
            require(reservation.token.generation > first.token.generation, "slot generation advances");
            require(pool.release_reader(first.token, 0) == Result::Invalid, "stale return rejected");
            found_original = true;
        }
    }
    require(found_original, "original slot is reclaimable after return");
    for (auto& reservation : reused) require(pool.abort(reservation) == Result::Ok, "release final reservation");
    pool.close_writer();
    std::cout << "PASS independent exec mappings, exact unlink lifetime, paused Reader loan, 10000 further commits, generation/stale-return\n";
}
}
int main(int argc, char** argv) {
    try {
        if (argc == 5 && std::string(argv[1]) == "--reader")
            return reader(argv[2], std::stoi(argv[3]), std::stoull(argv[4]));
        require(argc == 1, "usage: exec_demo"); parent(); return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
