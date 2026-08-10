#pragma once

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

namespace edb {

// ============================================================================
// ThreadSafeQueue — lock-based queue for cross-thread message passing
// ============================================================================
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // Non-copyable, non-movable
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // --- Push ---
    // Thread-safe: any thread can push.
    void Push(std::string msg) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(msg));
        }
        m_cv.notify_one();
    }

    // --- PushWithLimit ---
    // Thread-safe push with max queue size. If the queue is at capacity,
    // the oldest item is dropped to make room. Returns true if no item was dropped.
    // maxSize=0 means unlimited (always succeeds without dropping).
    bool PushWithLimit(std::string msg, size_t maxSize) {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool dropped = false;
        if (maxSize > 0 && m_queue.size() >= maxSize) {
            m_queue.pop();  // drop oldest
            dropped = true;
        }
        m_queue.push(std::move(msg));
        m_cv.notify_one();
        return !dropped;
    }

    // --- TryPop ---
    // Non-blocking. Returns true if an item was popped.
    bool TryPop(std::string& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return false;
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    // --- PopBlocking ---
    // Blocks until an item is available.
    std::string PopBlocking() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty(); });
        std::string result = std::move(m_queue.front());
        m_queue.pop();
        return result;
    }

    // --- Drain ---
    // Pop all available items without blocking. Returns number drained.
    // Items are appended to `out` via callback.
    template<typename F>
    size_t Drain(F&& callback) {
        size_t count = 0;
        std::string item;
        while (TryPop(item)) {
            callback(std::move(item));
            ++count;
        }
        return count;
    }

    // --- Size ---
    size_t Size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    // --- Empty ---
    bool Empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    mutable std::mutex m_mutex;
    std::queue<std::string> m_queue;
    std::condition_variable m_cv;
};

} // namespace edb
