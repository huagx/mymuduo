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
    using ThreadInitCallback = std::function<void(eventLoop *)>;

    EventLoopThreadPool(eventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    // 如果工作在多线程模式下， baseLoop_默认以轮询的方式分配channel给subloop
    eventLoop *getNextLoop();
    void setThreadNum(int numthreads);
    std::vector<eventLoop *> getAllLoops();
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    const std::string name() const { return name_; };

private:
    eventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<eventLoopThread>> threads_;
    std::vector<eventLoop *> loops_;
};