#include "epollPoller.h"
#include "logger.h"
#include "channel.h"
#include <cstring>
#include <unistd.h>

// channel未添加到poller中
const int kNew = -1;
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDelete = 2;

epollPoller::epollPoller(eventLoop *loop) : poller(loop), epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(kIntEventListSize)
{
    if (epollFd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d\n", errno);
    }
}

epollPoller::~epollPoller()
{
    close(epollFd_);
}

timeStamp epollPoller::poll(int timeoutMs, channelList *activeChannels)
{
    int numEvents = ::epoll_wait(epollFd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    timeStamp now(timeStamp::now());

    if (numEvents > 0)
    {
        LOG_DEBUG("%d events happend \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("no event happen.\n");
    }
    else
    {
        if (saveErrno != EINTR)
        {
            //
        }
    }

    return now;
}

void epollPoller::updateChannel(channel *chnl)
{
    const int index = chnl->index();
    LOG_INFO("fd=%d event=%d index=%d \n", chnl->fd(), chnl->events(), index);
    if (index == kNew || index == kDelete)
    {
        if (index == kNew)
        {
            int fd = chnl->fd();
            channels_[fd] = chnl;
        }
        chnl->setIndex(kAdded);
        update(EPOLL_CTL_ADD, chnl);
    }
    else
    {
        int fd = chnl->fd();
        if (chnl->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, chnl);
            chnl->setIndex(kDelete);
        }
        else
        {
            update(EPOLL_CTL_MOD, chnl);
        }
    }
}

void epollPoller::removeChannel(channel *chnl)
{
    int fd = chnl->fd();
    channels_.erase(fd);

    int index = chnl->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, chnl);
    }
    chnl->setIndex(kNew);
}

//填写活跃的连接
void epollPoller::fillActiveChannels(int numEvents, channelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        channel *chnl = static_cast<channel *>(events_[i].data.ptr);
        chnl->setRevents(events_[i].events);

        // eventLoop获取到了所有发生事件的channel列表。
        activeChannels->push_back(chnl);
    }
}

// epoll ctl add mod
void epollPoller::update(int operation, channel *chnl)
{
    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = chnl->events();
    event.data.ptr = chnl;
    int fd = chnl->fd();
    if (::epoll_ctl(epollFd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error! \n");
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error!\n");
        }
    }
}