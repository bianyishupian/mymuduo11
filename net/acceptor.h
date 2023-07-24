#pragma once

#include <functional>

#include "../base/noncopyable.h"
#include "socket.h"
#include "channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using newConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const newConnectionCallback &cb) { _newConnectionCallback = cb; }

    bool listenning() const { return _listenning; }
    void listen();

private:
    void handleRead();

    EventLoop *_loop;
    Socket _acceptSocket;
    Channel _acceptChannel;
    newConnectionCallback _newConnectionCallback;
    bool _listenning;
};