#include <sys/epoll.h>

#include "channel.h"
#include "eventloop.h"
#include "../base/logger.h"

const int Channel::NoneEvent = 0;
const int Channel::ReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::WriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : _loop(loop), _fd(fd), _events(0), _revents(0), _index(-1), _tied(false) {}

Channel::~Channel() {}

// channel的tie方法什么时候调用过?  TcpConnection => channel
/**
 * TcpConnection中注册了Chnanel对应的回调函数，传入的回调函数均为TcpConnection
 * 对象的成员方法，因此可以说明一点就是：Channel的结束一定早于TcpConnection对象！
 * 此处用tie去解决TcoConnection和Channel的生命周期时长问题，从而保证了Channel对
 * 象能够在TcpConnection销毁前销毁。
 **/
void Channel::tie(const std::shared_ptr<void> &obj)
{
    _tie = obj;
    _tied = true;
}

/**
 * 当改变channel所表示的fd的events事件后，update负责再poller里面更改fd相应的事件epoll_ctl
 **/
void Channel::update()
{
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    _loop->updateChannel(this);
}

// 在channel所属的EventLoop中把当前的channel删除掉
void Channel::remove()
{
    _loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (_tied)
    {
        std::shared_ptr<void> guard = _tie.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        // 如果提升失败了 就不做任何处理 说明Channel的TcpConnection对象已经不存在了
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d", _revents);
    // 关闭
    if ((_revents & EPOLLHUP) && !(_revents & EPOLLIN)) // 当TcpConnection对应Channel 通过shutdown 关闭写端 epoll触发EPOLLHUP
    {
        if (_closeCallback)
        {
            _closeCallback();
        }
    }
    // 错误
    if (_revents & EPOLLERR)
    {
        if (_errorCallback)
        {
            _errorCallback();
        }
    }
    // 读
    if (_revents & (EPOLLIN | EPOLLPRI))
    {
        if (_readCallback)
        {
            _readCallback(receiveTime);
        }
    }
    // 写
    if (_revents & EPOLLOUT)
    {
        if (_writeCallback)
        {
            _writeCallback();
        }
    }
}