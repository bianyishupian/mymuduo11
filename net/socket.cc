#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

#include "socket.h"
#include "../base/logger.h"
#include "inetaddress.h"

Socket::~Socket()
{
    close(_sockfd);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != bind(_sockfd, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail\n", _sockfd);
    }
}

void Socket::listen()
{
    if (::listen(_sockfd, 1024) != 0)   // 要加上::表示全局作用域的listen
    {
        LOG_FATAL("listen sockfd:%d fail\n", _sockfd);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     **/
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    // fixed : int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    int connfd = ::accept4(_sockfd, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if (shutdown(_sockfd, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}