#pragma once

#include <functional>
#include <memory>

#include "../base/noncopyable.h"
#include "../base/timestamp.h"

class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到Poller通知以后 处理事件 handleEvent在EventLoop::loop()中调用
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { _readCallback = std::move(cb); }
    void setWriteCallback(EventCallback cb) { _writeCallback = std::move(cb); }
    void setCloseCallback(EventCallback cb) { _closeCallback = std::move(cb); }
    void setErrorCallback(EventCallback cb) { _errorCallback = std::move(cb); }

    // 防止当channel被手动remove掉 channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return _fd; }
    int events() const { return _events; }
    void set_revents(int revt) { _revents = revt; }

    // 设置fd相应的事件状态 相当于epoll_ctl add delete
    void enableReading() { _events |= ReadEvent; update(); }
    void disableReading() { _events &= ~ReadEvent; update(); }
    void enableWriting() { _events |= WriteEvent; update(); }
    void disableWriting() { _events &= ~WriteEvent; update(); }
    void disableAll() { _events = NoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return _events == NoneEvent; }
    bool isWriting() const { return _events & WriteEvent; }
    bool isReading() const { return _events & ReadEvent; }

    int index() { return _index; }
    void set_index(int idx) { _index = idx; }

    // one loop per thread
    EventLoop *ownerLoop() { return _loop; }
    void remove();

private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int NoneEvent;
    static const int ReadEvent;
    static const int WriteEvent;

    EventLoop *_loop;
    const int _fd;
    int _events;
    int _revents;       // poller返回的事件
    int _index;

    std::weak_ptr<void> _tie;   // 延长生命周期
    bool _tied;

    ReadEventCallback _readCallback;
    EventCallback _writeCallback;
    EventCallback _closeCallback;
    EventCallback _errorCallback;
};
