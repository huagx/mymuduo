#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include "eventLoop.h"
#include "logger.h"
#include "poller.h"

__thread eventLoop *t_loopInthisthread = nullptr;

const int kPollTimes = 10000;

// 创建wakeupfd
static int createEventFd()
{
    int evtFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtFd < 0)
    {
        LOG_FATAL("create event fd error: %d\n", errno);
    }
    return evtFd;
}

eventLoop::eventLoop() : looping_(false), quit_(false), callPendingFunctors_(false), threadId_(currentThread::tid()), poller_(poller::newDefaultPoller(this)), wakeupFd_(createEventFd()), wakeupChannel_(new channel(this, wakeupFd_)), currentActiveChannel_(nullptr)
{
    if (t_loopInthisthread)
    {
        LOG_FATAL("another eventLoop %p exists in this thread %d \n", t_loopInthisthread, threadId_);
    }
    else
    {
        t_loopInthisthread = this;
    }

    //设置wakeupFd的事件类型
    wakeupChannel_->setReadCB(std::bind(&eventLoop::handleRead, this));

    //每一个eventloop都将监听wakeupfd的唤醒。
    wakeupChannel_->enableReading();
}

eventLoop::~eventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInthisthread = nullptr;
}

void eventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);

    if (n != sizeof one)
    {
        //
    }
}

void eventLoop::loop()
{
    quit_ = false;
    looping_ = true;

    LOG_INFO("eventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimes, &activeChannels_);

        for (auto &chnl : activeChannels_)
        {
            // poller能够监听那些channel发生事件了， 通知channel处理相应的事件。
            chnl->handleEvent(pollReturnTime_);
        }

        doPendingFunctors();
    }

    looping_ = false;
}

void eventLoop::quit()
{
    quit_ = true;

    // 如果是在其他线程中，调用的quit，在一个subloop中， 调用了mainLoop的quit
    if (!isInLoopThread())
    {
        wakeUp();
    }
}

void eventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    { //在非当前loop中执行cb
        queueInLoop(cb);
    }
}

//把cb队列放入线程中，唤醒loop所在的线程
void eventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    //唤醒需要执行回调的线程
    if (!isInLoopThread() || callPendingFunctors_)
    {
        wakeUp();
    }
}

void eventLoop::wakeUp()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        //
    }
}

void eventLoop::removeChannel(channel *chnl)
{
    poller_->removeChannel(chnl);
}

void eventLoop::unpdateChannel(channel *chnl)
{
    poller_->updateChannel(chnl);
}

bool eventLoop::hasChannel(channel *chnl)
{
    return poller_->hasChannel(chnl);
}

void eventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); //当前loop需要执行的回调操作
    }

    callPendingFunctors_ = false;
}