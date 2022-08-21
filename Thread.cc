#include <semaphore.h>
#include "Thread.h"
#include "currentThread.h"
std::atomic_int Thread::numCreated_(0);

Thread::Thread(threadFunc func, const std::string &name) 
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name) 
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach();
    }
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, 32, "Thread_%d", num);
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    thread_ = std::make_shared<std::thread>([&]()
                                            {
        tid_ = currentThread::tid(); 
        sem_post(&sem);
        func_(); });

    //等待上面线程创建完成
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}