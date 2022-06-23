#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue
{
    mutable std::mutex mtx_;
    std::condition_variable cv_not_empty_;
    std::queue<T> queue_;

public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

    void push(const T& item)
    {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            queue_.push(item);
        }

        cv_not_empty_.notify_one();
    }

    void push(T&& item)
    {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            queue_.push(std::move(item));
        }

        cv_not_empty_.notify_one();
    }

    void push(std::initializer_list<T> lst)
    {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            for (const auto& item : lst)
                queue_.push(item);
        }

        cv_not_empty_.notify_all();
    }

    bool try_pop(T& item)
    {
        std::unique_lock<std::mutex> lk {mtx_, std::try_to_lock};
        if (lk.owns_lock() && !queue_.empty())
        {
            if constexpr (std::is_nothrow_move_assignable_v<T>)
                item = std::move(queue_.front());
            else
                item = queue_.front();
                
            queue_.pop();
            return true;
        }
        return false;
    }

    void pop(T& item)
    {
        std::unique_lock<std::mutex> lk {mtx_};
        cv_not_empty_.wait(lk, [this] { return !queue_.empty(); });
        
        if constexpr (std::is_nothrow_move_assignable_v<T>)
            item = std::move(queue_.front());
        else
            item = queue_.front();
        
        queue_.pop();
    }
};

#endif // THREAD_SAFE_QUEUE_HPP
