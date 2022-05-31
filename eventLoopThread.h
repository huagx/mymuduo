#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "noncopyable.h"
#include "eventLoop.h"
#include "Thread.h"

class eventLoopThread : noncopyable
{
public:
    using threadInitCallback = std::function<void(eventLoop*)>;
    eventLoopThread(const threadInitCallback &cb = threadInitCallback(), const std::string &name = std::string());
    eventLoop *startLoop();
    ~eventLoopThread();
private:
    eventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    threadInitCallback callBack_;
    void threadFunc();

};

