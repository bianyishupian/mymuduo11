#include "eventloop.h"
#include "eventloopthread.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : _loop(nullptr), _exiting(false),
      _thread(std::bind(&EventLoopThread::threadFunc, this), name),
      _mutex(), _cond(), _callback(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    _exiting = true;
    if(_loop != nullptr)
    {
        _loop->quit();
        _thread.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    _thread.start();
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while(_loop == nullptr)
        {
            _cond.wait(lock);
        }
        loop = _loop;
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    if(_callback)
    {
        _callback(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _loop = &loop;
        _cond.notify_one();
    }
    loop.loop();
    std::unique_lock<std::mutex> lock(_mutex);
    _loop = nullptr;
}