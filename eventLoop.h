#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include "noncopyable.h"
#include "timeStamp.h"
#include "currentThread.h"
#include <vector>

class channel;
class poller;
class eventLoop : noncopyable
{
public:
    using Functor = std::function<void()>; 
    eventLoop();
    ~eventLoop();

    void loop(); //开启事件循环
    void quit(); //退出事件循环

    timeStamp pollReturnTime() const;

    void runInLoop(Functor cb); //在当前loop中执行循环
    void queueInLoop(Functor cb); //把cb放入队列中，唤醒loop所在的线程执行cb

    void wakeUp(); //用来唤醒loop所在的线程

    void updateChannel(channel *chnl);
    void removeChannel(channel *chnl);

    bool hasChannel(channel *chnl);
    bool isInLoopThread() const; //判断loop是否在自己的线程里面
private:
    using channelList = std::vector<channel*>;

    void handleRead(); //唤醒loop所在的线程的具体实现
    void doPendingFunctors();

    std::atomic_bool looping_;  //原子操作 底层通过CAS实现
    std::atomic_bool quit_; //退出loop操作

    const pid_t threadId_; //记录线程当前的线程id

    timeStamp pollReturnTime_;
    std::unique_ptr<poller> poller_;

    int wakeupFd_; //当mainloop获取一个新用户的channel，通过轮序算法选择一个subloop，通过该成员唤醒subloop.
    std::unique_ptr<channel> wakeupChannel_;

    channelList activeChannels_;
    channel *currentActiveChannel_;

    std::atomic_bool callPendingFunctors_;
    std::vector<Functor> pendingFunctors_; //存储loop存储需要的回调操作。
    std::mutex mutex_; //保护上面的函数队列线程安全
};