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
    void handleRead();  // 给eventfd返回的文件描述符wakeupFd_绑定的事件回调 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait
    void doPendingFunctors();   // 执行上层回调

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool _looping;  // 原子操作 标识在执行loop循环
    std::atomic_bool _quit;     // 原子操作 标识退出loop循环

    const pid_t _threadId;  // 标识了当前EventLoop的所属线程id

    Timestamp _pollReturnTime;  // Poller返回发生事件的Channels的时间点
    std::unique_ptr<Poller> _poller;    // 当前EventLoop拥有的poller

    int _wakeupFd;  // 作用：当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 通过该成员唤醒subLoop处理Channel
    std::unique_ptr<Channel> _wakeupChannel;

    ChannelList _activeChannels;    // 返回Poller检测到当前有事件发生的所有Channel列表

    std::atomic_bool _callingPendingFunctors;   // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> _pendingFunctors;      // 存储loop需要执行的所有回调操作
    std::mutex _mutex;                          // 互斥锁 用来保护上面vector容器的线程安全操作
};
