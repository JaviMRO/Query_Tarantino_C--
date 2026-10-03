#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

namespace tarantino::platform {

/// An unbounded queue whose take waits for an element, as Java's LinkedBlockingQueue: any thread adds, one takes.
template <typename Element>
class BlockingQueue {
public:
    void add(Element element) {
        {
            const std::lock_guard lock(mutex_);
            elements_.push_back(std::move(element));
        }
        available_.notify_one();
    }

    Element take() {
        std::unique_lock lock(mutex_);
        available_.wait(lock, [this] { return !elements_.empty(); });
        Element element = std::move(elements_.front());
        elements_.pop_front();
        return element;
    }

private:
    std::mutex mutex_;
    std::condition_variable available_;
    std::deque<Element> elements_;
};

}
