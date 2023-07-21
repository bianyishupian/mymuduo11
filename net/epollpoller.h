#pragma once

#include <vector>
#include <sys/epoll.h>

#include "mymuduo11/net/poller.h"
#include "mymuduo11/base/timestamp.h"

class Channel;

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int initEventListSize = 16;
    void fillActiveChannels(int numEvents, ChannelList *activeChannels);

    void update(int opreation, Channel *Channel);
    using EventList = std::vector<epoll_event>;

    int _epollfd;
    EventList _events;
};