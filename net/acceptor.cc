#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "acceptor.h"
#include "../base/logger.h"
#include "inetaddress.h"

static int createNonblocking()
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
: _loop(loop), _acceptSocket(createNonblocking())
, _acceptChannel(loop, _acceptSocket.fd()), _listenning(false)
{
    _acceptSocket.setReuseAddr(true);
    _acceptSocket.setReusePort(true);
    _acceptSocket.bindAddress(listenAddr);
    _acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor::~Acceptor()
{
    _acceptChannel.disableAll();
    _acceptChannel.remove();
}

void Acceptor::listen()
{
    _listenning = true;
    _acceptSocket.listen();
    _acceptChannel.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = _acceptSocket.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(_newConnectionCallback)
        {
            _newConnectionCallback(connfd, peerAddr);
        }
        else
        {
            close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}