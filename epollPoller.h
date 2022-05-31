#pragma once

#include "poller.h"
#include "noncopyable.h"
#include "timeStamp.h"
#include <vector>
#include <sys/epoll.h>

class epollPoller : public poller
{
public:
    epollPoller(eventLoop *loop);
    ~epollPoller() override;

    //重写基类的抽象方法
    timeStamp poll(int timeoutMs, channelList *activeChannels) override;
    void updateChannel(channel *channel) override;
    void removeChannel(channel *chnl) override;

private:
    static const int kIntEventListSize = 16;

    //填写活跃的连接
    void fillActiveChannels(int numEvents, channelList *activeChannels) const;
    void update(int operation, channel *chnl);
    using eventList = std::vector<epoll_event>;

    int epollFd_;
    eventList events_;
    
};
