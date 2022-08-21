#include "EventLoopThreadPool.h"
#include "eventLoopThread.h"
#include "eventLoop.h"

EventLoopThreadPool::EventLoopThreadPool(eventLoop *loop, const std::string &nameArg) 
    : baseLoop_(loop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0) 
{

}

EventLoopThreadPool::~EventLoopThreadPool() 
{

}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        eventLoopThread *t = new eventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<eventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }

    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

std::vector<eventLoop*> EventLoopThreadPool::getAllLoops() 
{
    if (loops_.empty())
    {
        return std::vector<eventLoop*>(1, baseLoop_);
        //return {baseLoop_};
    } 
    else 
    {
        return loops_;
    }
}

eventLoop *EventLoopThreadPool::getNextLoop() 
{
    eventLoop *loop = baseLoop_;
    
    if (!loops_.empty()) 
    {
        loop = loops_[next_];
        next_ = (next_+1) % loops_.size();
    }
    return loop;
}