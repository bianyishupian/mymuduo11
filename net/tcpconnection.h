#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "inetaddress.h"
#include "buffer.h"
#include "callbacks.h"
#include "../base/noncopyable.h"
#include "../base/timestamp.h"

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd, 
                  const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return _loop; }
    const std::string &name() const { return _name; }
    const InetAddress &localAddr() const { return _localAddr; }
    const InetAddress &peerAddr() const { return _peerAddr; }

    bool connected() const { return _state == kConnected; }

    void send(const std::string &buf);

    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb) { _connectionCallback = cb; }
    void setMessageCallback(const MessageCallback &cb) { _messageCallback = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { _writeCompleteCallback = cb; }
    void setCloseCallback(const CloseCallback &cb) { _closeCallback = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark) { _highWaterMarkCallback = cb; _highWaterMark = highWaterMark; }

    void connectEstablished();  // 连接建立
    void connectDestroyed();    // 连接销毁

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    void setState(StateE state) { _state = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    
    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();

    EventLoop *_loop;
    const std::string _name;
    std::atomic_int _state;
    bool _reading;

    std::unique_ptr<Socket> _socket;
    std::unique_ptr<Channel> _channel;

    const InetAddress _localAddr;
    const InetAddress _peerAddr;

    // 这些回调TcpServer也有 用户通过写入TcpServer注册 TcpServer再将注册的回调传递给TcpConnection TcpConnection再将回调注册到Channel中
    ConnectionCallback _connectionCallback;       // 有新连接时的回调
    MessageCallback _messageCallback;             // 有读写消息时的回调
    WriteCompleteCallback _writeCompleteCallback; // 消息发送完成以后的回调
    HighWaterMarkCallback _highWaterMarkCallback;
    CloseCallback _closeCallback;
    size_t _highWaterMark;

    // buffer
    Buffer _inputBuffer;
    Buffer _outputBuffer;
};