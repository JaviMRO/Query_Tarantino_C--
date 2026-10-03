#include "tarantino/platform/ThreadPool.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>

namespace tarantino::platform {
namespace {

TEST(ThreadPoolTest, runsEveryTask) {
    std::atomic<int> done{0};
    std::promise<void> allDone;
    {
        ThreadPool pool(3);
        for (int task = 0; task < 20; ++task) {
            pool.execute([&] {
                if (++done == 20) {
                    allDone.set_value();
                }
            });
        }
        allDone.get_future().wait();
    }

    EXPECT_EQ(20, done.load());
}

TEST(ThreadPoolTest, runsAsManyTasksAtOnceAsItHasThreads) {
    std::atomic<int> running{0};
    std::atomic<int> mostRunning{0};
    std::atomic<int> finished{0};
    {
        ThreadPool pool(3);
        for (int task = 0; task < 9; ++task) {
            pool.execute([&] {
                const int now = ++running;
                int most = mostRunning.load();
                while (now > most && !mostRunning.compare_exchange_weak(most, now)) {
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                --running;
                ++finished;
            });
        }
        while (finished < 9) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    EXPECT_EQ(3, mostRunning.load());
}

TEST(ThreadPoolTest, rejectsAPoolWithoutThreads) {
    EXPECT_THROW(ThreadPool(0), std::invalid_argument);
}

}
}
