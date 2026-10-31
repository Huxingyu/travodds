#include "gtest/gtest.h"
#include "common/threads/threadpool.h"
#include <chrono>
#include <future>
#include <thread>

USING_TRAVODDS_NAMESPACE
using namespace std::chrono_literals;

TEST(ThreadPoolProgress, NestedTaskCanCompleteOnAnotherWorker) {
    ThreadPool pool(2);
    auto outer = pool.submit([&pool] {
        auto inner = pool.submit([] { return 37; });
        // A finite wait makes the old serialized-worker behavior fail without
        // leaving either worker blocked forever during test cleanup.
        return inner.wait_for(1s) == std::future_status::ready && inner.get() == 37;
    });
    ASSERT_EQ(outer.wait_for(3s), std::future_status::ready);
    EXPECT_TRUE(outer.get());
    pool.shutdown();
}

TEST(ThreadPoolProgress, ImmediateAndIdleShutdownNeverLoseTheWakeup) {
    for (int iteration = 0; iteration < 512; ++iteration) {
        SCOPED_TRACE(iteration);
        ThreadPool pool(1);
        if (iteration % 2) {
            // Race shutdown against a worker returning to its idle wait.
            auto completed = pool.submit([] {});
            ASSERT_EQ(completed.wait_for(1s), std::future_status::ready);
            completed.get();
            std::this_thread::yield();
        }
        pool.shutdown();
    }
}

TEST(ThreadPoolProgress, SubmissionAfterShutdownReturnsReadyBrokenPromise) {
    ThreadPool pool(1);
    pool.shutdown();
    auto rejected = pool.submit([] { return 51; });
    ASSERT_EQ(rejected.wait_for(0ms), std::future_status::ready);
    try {
        static_cast<void>(rejected.get());
        FAIL() << "Submission to a stopped pool unexpectedly executed";
    } catch (const std::future_error& error) {
        EXPECT_EQ(error.code(), std::make_error_code(std::future_errc::broken_promise));
    }
}
