#pragma once
#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

#include "../base/noncopyable.h"
#include "../base/thread.h"

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();

    EventLoop *_loop;
    bool _exiting;
    Thread _thread;
    std::mutex _mutex;
    std::condition_variable _cond;
    ThreadInitCallback _callback;
};