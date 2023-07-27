#include <sys/eventfd.h> // eventfd
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "../net/eventloop.h"
#include "../net/poller.h"
#include "../net/channel.h"
#include "../base/logger.h"

// __thread : 线程局部变量
__thread EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000; // 10s

int createEventfd()
{
    int evfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (evfd < 0)
    {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evfd;
}

// TODO
EventLoop::EventLoop()
    : _looping(false), _quit(false), _callingPendingFunctors(false),
      _threadId(CurrentThread::tid()), _poller(Poller::newDefaultPoller(this)),
      _wakeupFd(createEventfd()), _wakeupChannel(new Channel(this, _wakeupFd))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, _threadId);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, _threadId);
    }
    else
    {
        t_loopInThisThread = this;
    }
    _wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    _wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
    _wakeupChannel->disableAll();
    _wakeupChannel->remove();
    close(_wakeupFd);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    _looping = true;
    _quit = false;

    LOG_INFO("EventLoop %p start looping\n", this);

    while(!_quit)
    {
        _activeChannels.clear();
        _pollReturnTime = _poller->poll(kPollTimeMs, &_activeChannels);
        for(auto channel : _activeChannels)
        {
            channel->handleEvent(_pollReturnTime);
        }
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping.\n", this);
    _looping = false;
}
void EventLoop::quit()
{
    _quit = true;
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _pendingFunctors.emplace_back(cb);
    }

    if(!isInLoopThread() || _callingPendingFunctors)
    {
        wakeup();
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n =write(_wakeupFd, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

// EventLoop的方法 => Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    _poller->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    _poller->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return _poller->hasChannel(channel);
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(_wakeupFd, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
    }
}



void EventLoop::doPendingFunctors() // 执行上层回调
{
    std::vector<Functor> functors;
    _callingPendingFunctors = true;
    
    {
        std::unique_lock<std::mutex> lock(_mutex);
        functors.swap(_pendingFunctors);
    }
    for(const Functor &functor : functors)
    {
        functor();
    }

    _callingPendingFunctors = false;
}