#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "mymuduo11/base/noncopyable.h"
#include "mymuduo11/base/timestamp.h"
#include "mymuduo11/base/currentthread.h"

class Channel;
class Poller;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    void loop();    // 开启事件循环
    void quit();    // 退出事件循环

    Timestamp pollReturnTime() const { return _pollReturnTime; }

    // 在当前loop中执行
    void runInLoop(Functor cb);
    // 把上层注册的回调函数cb放入队列中 唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);

    // 通过eventfd唤醒loop所在的线程
    void wakeup();

    // EventLoop的方法 => Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程里
    bool isInLoopThread() const { return _threadId == CurrentThread::tid(); } // threadId_为EventLoop创建时的线程id CurrentThread::tid()为当前线程id

private:
    void handleRdad();
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool _looping;
    std::atomic_bool _quit;

    const pid_t _threadId;

    Timestamp _pollReturnTime;
    std::unique_ptr<Poller> _poller;

    int _wakeupFd;
    std::unique_ptr<Channel> _wakeupChannel;

    ChannelList _activeChannels;

    std::atomic_bool _callingPendingFunctors;
    std::vector<Functor> _pendingFunctors;
    std::mutex _mutex;
};
