#pragma once

#include <unordered_map>

#include "eventloop.h"
#include "acceptor.h"
#include "inetaddress.h"
#include "eventloopthreadpool.h"
#include "callbacks.h"
#include "tcpconnection.h"
#include "buffer.h"
#include "../base/noncopyable.h"

class TcpServer
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = kNoReusePort);
    ~TcpServer();
    void setThreadInitCallback(const ThreadInitCallback &cb) { _threadInitCallback = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { _connectionCallback = cb; }
    void setMessageCallback(const MessageCallback &cb) { _messageCallback = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { _writeCompleteCallback = cb; }

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);
    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *_loop;
    const std::string _ipPort;
    const std::string _name;

    std::unique_ptr<Acceptor> _acceptor;
    std::shared_ptr<EventLoopThreadPool> _threadPool;

    ConnectionCallback _connectionCallback;
    MessageCallback _messageCallback;
    WriteCompleteCallback _writeCompleteCallback;
    ThreadInitCallback _threadInitCallback;

    std::atomic_int _started;
    int _nextConnId;
    ConnectionMap _connections;
};