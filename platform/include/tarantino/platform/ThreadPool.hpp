#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace tarantino::platform {

/// A fixed pool of threads that run tasks in the order they are submitted, as Java's Executors.newFixedThreadPool:
/// the control layer downloads with a pool of D threads (SPEC §9, §14). Tasks report their own failures.
class ThreadPool {
public:
    explicit ThreadPool(int threads);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// Drops the tasks that have not started and waits for the running ones.
    ~ThreadPool();

    void execute(std::function<void()> task);

private:
    void work();

    std::mutex mutex_;
    std::condition_variable available_;
    std::deque<std::function<void()>> tasks_;
    bool stopping_ = false;
    std::vector<std::thread> threads_;
};

}
