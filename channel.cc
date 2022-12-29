#include <sys/epoll.h>
#include "channel.h"
#include "Logger.h"
#include "eventLoop.h"

const int channel::KNoneEvent = 0;
const int channel::KReadEvent = EPOLLIN | EPOLLPRI;
const int channel::KWriteEvent = EPOLLOUT;

void channel::enableReading()
{
    events_ |= KReadEvent;
    update();
}

void channel::disableReading()
{
    events_ &= ~KReadEvent;
    update();
}

void channel::enableWriteing()
{
    events_ |= KWriteEvent;
    update();
}

void channel::disableWriteing()
{
    events_ &= ~KWriteEvent;
    update();
}

void channel::disableAll()
{
    events_ = KNoneEvent;
    update();
}

bool channel::isNoneEvent() const
{
    return events_ == KNoneEvent;
}

bool channel::isWriting() const
{
    return events_ == KWriteEvent;
}

bool channel::isReading() const
{
    return events_ == KReadEvent;
}

int channel::index() const
{
    return index_;
}

void channel::setIndex(int index)
{
    index_ = index;
}

void channel::setReadCB(readEventCallback cb)
{
    readCb_ = cb;
}

void channel::setWriteCB(eventCallback cb)
{
    writeCb_ = cb;
}

void channel::setCloseCB(eventCallback cb)
{
    closeCb_ = cb;
}

void channel::setErrorCB(eventCallback cb)
{
    errorCb_ = cb;
}

void channel::setRevents(int events)
{
    revents_ = events;
}

int channel::fd() const
{
    return fd_;
}
int channel::events() const
{
    return events_;
}

eventLoop *channel::ownerLoop() const
{
    return loop_;
}

channel::channel(eventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

channel::~channel()
{
}

void channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

//当改变channel所表示fd的events事件后， update负责在poller里面更改fd相应的事件
void channel::update()
{
    //通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件。
    loop_->updateChannel(this);  
}

//在channel所属的eventloop中删除当前channel
void channel::remove()
{
    loop_->removeChannel(this);
}

void channel::handleEvent(timeStamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void channel::handleEventWithGuard(timeStamp receiveTime)
{
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCb_)
        {
            closeCb_();
        }
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCb_)
        {
            errorCb_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCb_)
        {
            readCb_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT)
    {
        if (writeCb_)
        {
            writeCb_();
        }
    }
}