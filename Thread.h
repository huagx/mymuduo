#pragma once 

#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include "noncopyable.h"
class Thread : noncopyable
{

public:
    using threadFunc = std::function<void()>;

    explicit Thread(threadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();
    bool started() const;
    pid_t tid() const;
    const std::string& name() const;
    static const int numCreated();
private:
    void setDefaultName();
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    threadFunc func_;
    std::string name_;
    static std::atomic_int numCreated_;
};
