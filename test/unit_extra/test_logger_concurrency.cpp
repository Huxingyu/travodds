#include "common/log/logger.h"
#include "gtest/gtest.h"
#include <atomic>
#include <cstdlib>
#include <thread>
#include <vector>

USING_TRAVODDS_NAMESPACE

namespace {

#if GTEST_HAS_DEATH_TEST
int exerciseConcurrentLogger(bool concurrentShutdown)
{
    // Set the environment before creating threads. Masking output still
    // exercises LogToConsole's implicit initialization on every call.
#ifdef _WIN32
    if (_putenv_s("TRAVODDS_LOG_MASK", "console:0,file:0") != 0) return 2;
#else
    if (setenv("TRAVODDS_LOG_MASK", "console:0,file:0", 1) != 0) return 2;
#endif
    for (unsigned round = 0; round < 16; ++round) {
        LogDestroy();
        std::atomic<unsigned> ready{0};
        std::atomic<bool> go{false};
        std::vector<std::thread> workers;
        for (unsigned i = 0; i < 32; ++i) {
            workers.emplace_back([&, i] {
                ready.fetch_add(1);
                while (!go.load()) std::this_thread::yield();
                if (concurrentShutdown && i % 2 == 0) {
                    LogDestroy();
                } else {
                    LogToConsole(LOG_INFO, __FILE__, __func__, __LINE__,
                        "concurrent logger startup");
                }
            });
        }
        while (ready.load() != workers.size()) std::this_thread::yield();
        go = true;
        for (auto& worker : workers) worker.join();
        LogDestroy();
    }
    return 0;
}
#endif

} // namespace

TEST(CommonLogConcurrencyDeathTest, ConcurrentFirstUseAndRestart)
{
#if GTEST_HAS_DEATH_TEST
    // Re-exec so the logger singleton is fresh and an abort cannot kill the
    // remaining tests. Google Test restores its flags after each test.
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_EXIT(std::_Exit(exerciseConcurrentLogger(false)),
        ::testing::ExitedWithCode(0), "");
#else
    GTEST_SKIP() << "Subprocess tests are unavailable on this platform";
#endif
}

TEST(CommonLogConcurrencyDeathTest, ConcurrentInitializationAndShutdown)
{
#if GTEST_HAS_DEATH_TEST
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_EXIT(std::_Exit(exerciseConcurrentLogger(true)),
        ::testing::ExitedWithCode(0), "");
#else
    GTEST_SKIP() << "Subprocess tests are unavailable on this platform";
#endif
}
