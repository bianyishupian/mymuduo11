#include <errno.h>
#include <unistd.h>
#include <string.h>
// #include <assert.h>

#include "../base/logger.h"
#include "../net/channel.h"
#include "../net/epollpoller.h"

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop), _epollfd(epoll_create1(EPOLL_CLOEXEC)), _events(initEventListSize)
{
    if (_epollfd < 0)
        LOG_FATAL("epoll_create1 error:%d \n", errno);
}

EpollPoller::~EpollPoller()
{
    close(_epollfd);
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_INFO("func=%s => fd total count:%lu", __FUNCTION__, _channels.size());
    int numEvents = epoll_wait(_epollfd, &*_events.begin(), static_cast<int>(_events.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d events happend", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == _events.size())
        {
            _events.resize(_events.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout!", __FUNCTION__);
    }
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EpollPoller::poll() error!\n");
        }
    }
    return now;
}

void EpollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d", __FUNCTION__, channel->fd(), channel->events(), index);
    if (index == kNew || index == kDeleted)
    {
        
        if (index == kNew)
        {
            int fd = channel->fd();
            _channels[fd] = channel;
        }
        else
        {
            // assert(_channels.find(fd) != _channels.end());
            // assert(_channels[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        // (void)fd;
        // assert(_channels.find(fd) != _channels.end());
        // assert(_channels[fd] == channel);
        // assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从Poller中删除channel
void EpollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    _channels.erase(fd);

    LOG_INFO("func=%s => fd=%d", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(_events[i].data.ptr);
        channel->set_revents(_events[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));

    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (epoll_ctl(_epollfd, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}