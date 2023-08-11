#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class LockQueue
{
public:
    void Push(const T &data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(data);
        cv_.notify_one();
    }
    T Pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(queue_.empty())
        {
            cv_.wait(lock);
        }
        T data = queue_.front();
        queue_.pop();
        return data;
    }
private:
    std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cv_;
};