#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

#include "tcpconnection.h"
#include "socket.h"
#include "channel.h"
#include "eventloop.h"
#include "../base/logger.h"

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd,
                             const InetAddress &localAddr, const InetAddress &peerAddr)
    : _loop(CheckLoopNotNull(loop)), _name(nameArg), _state(kConnecting), _reading(true), _socket(new Socket(sockfd)),
      _channel(new Channel(loop, sockfd)), _localAddr(localAddr), _peerAddr(peerAddr), _highWaterMark(64 * 1024 * 1024)
{
    _channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    _channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    _channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    _channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d", _name.c_str(), sockfd);
    _socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d", _name.c_str(), _channel->fd(), (int)_state);
}

void TcpConnection::send(const std::string &buf)
{
    if (_state == kConnected)
    {
        if (_loop->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            _loop->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;     // 已经发送的数据长度
    size_t remaining = len; // 还剩下多少数据需要发送
    bool faultError = false;
    if (_state == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing\n");
    }
    // 表示channel_第一次开始写数据或者缓冲区没有待发送数据
    // 如果注册了可写事件并且之前的数据已经发送完毕
    if (!_channel->isWriting() && _outputBuffer.readableBytes() == 0)
    {
        nwrote = write(_channel->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && _writeCompleteCallback)
            {
                _loop->queueInLoop(std::bind(_writeCompleteCallback, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop\n");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }
    // 如果数据没有全部发送出去，则把剩余的数据都添加到outputBuffer_中，并向Epoller中注册可写事件
    if (!faultError && remaining > 0)
    {
        size_t oldLen = _outputBuffer.readableBytes(); // 目前发送缓冲区剩余的待发送的数据的长度
        // 判断待写数据是否会超过设置的高位标志highWaterMark_
        if (oldLen + remaining >= _highWaterMark && oldLen < _highWaterMark && _highWaterMarkCallback)
        {
            _loop->queueInLoop(std::bind(_highWaterMarkCallback, shared_from_this(), oldLen + remaining));
        }
        // 将data中剩余还没有发送的数据最佳到buffer中
        _outputBuffer.append((char *)data + nwrote, remaining);
        if (!_channel->isWriting())
        {
            _channel->enableWriting(); // 这里一定要注册channel的写事件 否则Epoller不会给channel通知epollout
        }
    }
}

void TcpConnection::shutdown()
{
    if (_state == kConnected)
    {
        setState(kDisconnecting);
        _loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!_channel->isWriting())
    {
        _socket->shutdownWrite();
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);    // 建立连接，设置一开始状态为连接态
    _channel->tie(shared_from_this());
    _channel->enableReading();  // 向Epoller注册channel的EPOLLIN读事件

    // 新连接建立 执行回调
    _connectionCallback(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if (_state == kConnected)
    {
        setState(kDisconnected);
        _channel->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
        _connectionCallback(shared_from_this());
    }
    _channel->remove(); // 把channel从epoller中注销掉
}

/*
    其实该函数就是把TCP可读缓冲区的数据读入到inputBuffer_中，以腾出TCP可读缓冲区，避免反复
    触发EPOLLIN事件（可读事件），同时执行用户自定义的消息到来时候的回调函数messageCallback_。
*/
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    // 把TCP缓冲区的数据读入到inputBuffer_中
    ssize_t n = _inputBuffer.readFd(_channel->fd(), &savedErrno);
    if (n > 0) // 从fd读到了数据，并且放在了inputBuffer_上
    {
        // 已建立连接的用户有可读事件发生了 调用用户传入的回调操作onMessage shared_from_this就是获取了TcpConnection的智能指针
        _messageCallback(shared_from_this(), &_inputBuffer, receiveTime);
    }
    // n=0表示对方关闭了
    else if (n == 0)
    {
        handleClose();
    }
    else // 出错了
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead\n");
        handleError();
    }
}
void TcpConnection::handleWrite()
{
    if (_channel->isWriting()) // 如果该channel在EPoller上注册了可写事件
    {
        int savedErrno = 0;
        // 把outputBuffer_中的数据发送出去
        ssize_t n = _outputBuffer.writeFd(_channel->fd(), &savedErrno);
        if (n > 0)
        {
            _outputBuffer.retrieve(n); // 把outputBuffer_的readerIndex往前移动n个字节，因为outputBuffer_中readableBytes已经发送出去了n字节
            // 如果数据全部发送完毕
            if (_outputBuffer.readableBytes() == 0)
            {
                _channel->disableWriting(); // 数据发送完毕后注销写事件，以免epoll频繁触发可写事件，导致效力低下
                if (_writeCompleteCallback)
                {
                    _loop->queueInLoop(std::bind(_writeCompleteCallback, shared_from_this()));
                }
                if (_state == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite\n");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing\n", _channel->fd());
    }
}
/*
    TcpConnection::handleClose函数就是把该连接对应Chanel上所有感兴趣的事件都从EPoller中注销，
    并且将自己从TcpServer中存储的所有连接中移除
*/
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d", _channel->fd(), (int)_state);
    setState(kDisconnected);
    _channel->disableAll(); // 注销所有感兴趣的事件
    TcpConnectionPtr connPtr(shared_from_this());
    _connectionCallback(connPtr); // 执行连接关闭的回调(用户自定的，而且和新连接到来时执行的是同一个回调)
    // 将自己从TcpServer中存储的所有连接中移除
    _closeCallback(connPtr); // 执行关闭连接的回调 执行的是TcpServer::removeConnection回调方法   // must be the last line
}
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (getsockopt(_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", _name.c_str(), err);
}