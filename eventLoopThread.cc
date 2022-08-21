#include "eventLoopThread.h"

eventLoopThread::eventLoopThread(const threadInitCallback &cb, const std::string &name) 
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&eventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callBack_(cb)
{

}

eventLoopThread::~eventLoopThread() 
{
    exiting_ = true;
    if (loop_ != nullptr) 
    {
        loop_->quit();
        thread_.join();
    }
}

eventLoop *eventLoopThread::startLoop() 
{
    thread_.start(); //启动线程循环

    eventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr) 
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}

void eventLoopThread::threadFunc() 
{
    eventLoop loop; //在创建线程时创建一个eventloop，和上面的线程一一对应 one loop per thread

    if (callBack_) 
    {
        callBack_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}