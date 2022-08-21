#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include "eventLoop.h"
#include "logger.h"
#include "poller.h"
#include "channel.h"

// 防止一个线程创建多个EventLoop
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

eventLoop::eventLoop() 
    : looping_(false)
    , quit_(false)
    , callPendingFunctors_(false)
    , threadId_(currentThread::tid())
    , poller_(poller::newDefaultPoller(this))
    , wakeupFd_(createEventFd())
    , wakeupChannel_(new channel(this, wakeupFd_))
    , currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
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
        LOG_ERROR("eventLoop::handleRead reads %d bytes instead of 8", n);
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

        for (auto chnl : activeChannels_)
        {
            // poller能够监听那些channel发生事件了， 通知channel处理相应的事件。 如果这个loop是mainLoop的话，那么这里处理的是accept事件
            chnl->handleEvent(pollReturnTime_);
        }

        // main Loop accept fd => channel subloop
        // mainLoop 注册的回调需要subloop执行
        doPendingFunctors();
    }

    LOG_INFO("eventLoop::loop %p stop looping. \n", this);
    looping_ = false;
}

void eventLoop::quit()
{
    quit_ = true;

    // 如果是在其他线程（loop）中，调用的mainLoop的quit
    // 此时mainloop有可能处于epoll状态，则需要调用mainloop的wakeup函数将mainloop唤醒
    if (!isInLoopThread())
    {
        wakeUp();
    }
}

void eventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) //在当前的loop线程中，直接执行回调
    {
        cb();
    }
    else //在非当前loop线程中执行cb
    { 
        queueInLoop(cb);
    }
}

//把cb队列放入线程中，唤醒loop所在的线程
void eventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);  //有可能有其他的多个loop将回调放入当前loop的队列中，所以需要加锁控制下
        pendingFunctors_.emplace_back(cb);
    }
    //唤醒需要执行回调的线程
    /**
     * @brief isInLoopThread ， 当loop不在当前的线程中时需要唤醒当前的线程去执行回调， 
     * callPendingFunctors_的条件意义为： 当前loop正在执行回调函数，此时callPendingFunctors_为false,而现在又有了新的回调函数，所以需要再唤醒当前loop
     */
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
        LOG_DEBUG("eventLoop::wakeUp() writes %lu bytes instead of 8 \n" n);
    }
}

void eventLoop::removeChannel(channel *chnl)
{
    poller_->removeChannel(chnl);
}

void eventLoop::updateChannel(channel *chnl)
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

    //采用局部变量一次性将任务队列中的任务全部获取，避免一次去取一条。 
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