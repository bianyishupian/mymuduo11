#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "../base/noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool() {}

    void setThreadNum(int numThreads) { _numThreads = numThreads; }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    
    // 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return _started; }
    const std::string name() const { return _name; }

private:
    EventLoop *_baseLoop;
    std::string _name;
    bool _started;
    int _numThreads;
    int _next;  // 轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> _threads;
    std::vector<EventLoop *> _loops;
};