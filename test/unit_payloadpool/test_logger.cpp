#include "gtest/gtest.h"
#include "common/log/logger.h"
#include <atomic>
#include <thread>
#include <vector>
#if defined(__linux__)
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
extern char** environ;
#endif

USING_TRAVODDS_NAMESPACE

TEST(LoggerSynchronization, ConcurrentFirstUseAndRestartAcrossExec) {
#if defined(__linux__)
    if (!std::getenv("TRAVODDS_LOGGER_CHILD")) {
        // Isolate first use from logging performed by earlier DDS tests.
        std::vector<std::string> environment;
        for (auto** p = environ; *p; ++p) environment.emplace_back(*p);
        environment.emplace_back("TRAVODDS_LOGGER_CHILD=1");
        environment.emplace_back("TRAVODDS_LOG_MASK=console:0,file:0");
        std::vector<char*> env;
        for (auto& value : environment) env.push_back(value.data());
        env.push_back(nullptr);
        char executable[] = "/proc/self/exe";
        char filter[] = "--gtest_filter=LoggerSynchronization.ConcurrentFirstUseAndRestartAcrossExec";
        char* args[] = {executable, filter, nullptr};
        pid_t child = -1;
        ASSERT_EQ(posix_spawn(&child, executable, nullptr, nullptr, args, env.data()), 0);
        int status = 0;
        ASSERT_EQ(waitpid(child, &status, 0), child);
        ASSERT_TRUE(WIFEXITED(status)) << "child status=" << status;
        EXPECT_EQ(WEXITSTATUS(status), 0);
        return;
    }
#endif
    for (unsigned round = 0; round < 16; ++round) {
        std::atomic<unsigned> ready{0};
        std::atomic<bool> go{false};
        std::vector<std::thread> workers;
        for (unsigned i = 0; i < 32; ++i) workers.emplace_back([&] {
            ++ready;
            while (!go.load()) std::this_thread::yield();
            LogToConsole(LOG_INFO, __FILE__, __func__, __LINE__, "concurrent first log");
        });
        while (ready.load() != workers.size()) std::this_thread::yield();
        go = true;
        for (auto& worker : workers) worker.join();
        ASSERT_EQ(LogDestroy(), 0); // Drain, then repeat simultaneous startup.
    }
}
