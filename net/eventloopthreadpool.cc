#include <memory>

#include "eventloopthread.h"
#include "eventloopthreadpool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
: _baseLoop(baseLoop), _name(nameArg), _started(false), _numThreads(0), _next(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    _started = true;
    for(int i = 0; i < _numThreads; ++i)
    {
        char buf[_name.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", _name.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        _threads.push_back(std::unique_ptr<EventLoopThread>(t));
        _loops.push_back(t->startLoop());
    }
    if(_numThreads == 0 && cb)
    {
        cb(_baseLoop);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = _baseLoop;
    if(!_loops.empty())
    {
        loop = _loops[_next];
        ++_next;
        if(_next >= _loops.size())
        {
            _next = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if(_loops.empty())
    {
        return std::vector<EventLoop*>(1, _baseLoop);
    }
    else
    {
        return _loops;
    }
}