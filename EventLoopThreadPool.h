#pragma once
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class eventLoop;
class eventLoopThread;

class EventLoopThreadPool 
{
public: 
    using ThreadInitCallback = std::function<void(eventLoop*)>;

    EventLoopThreadPool(eventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    eventLoop *getNextLoop();
    void setThreadNum(int numthreads);
    std::vector<eventLoop*> getAllLoops();
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
private:
    eventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<eventLoopThread>> threads_;
    std::vector<eventLoop*> loops_;
};