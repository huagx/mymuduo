#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <sstream>
#include <stdio.h>
#include <string>
#include <queue>
#include <memory>
#include <unistd.h>
#include <sys/syscall.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

namespace mymuduo 
{

#define LogDebug \
	if (mymuduo::LogLevel::DEBUG >= mymuduo::LogLevel::DEBUG) \
		mymuduo::Logger::ptr(new mymuduo::Logger(mymuduo::LogLevel::DEBUG, __FILE__, __LINE__, __func__))->stream()

#define LogInfo \
	if (mymuduo::LogLevel::INFO >= mymuduo::LogLevel::DEBUG) \
		mymuduo::Logger::ptr(new mymuduo::Logger(mymuduo::LogLevel::INFO, __FILE__, __LINE__, __func__))->stream()

#define LogWarn \
	if (mymuduo::LogLevel::WARN >= mymuduo::LogLevel::DEBUG) \
		mymuduo::Logger::ptr(new mymuduo::Logger(mymuduo::LogLevel::WARN, __FILE__, __LINE__, __func__))->stream()

#define LogError \
	if (mymuduo::LogLevel::ERROR >= mymuduo::LogLevel::DEBUG) \
		mymuduo::Logger::ptr(new mymuduo::Logger(mymuduo::LogLevel::ERROR, __FILE__, __LINE__, __func__))->stream()

#define LogFatal \
	if (mymuduo::LogLevel::FITAL >= mymuduo::LogLevel::DEBUG) \
		mymuduo::Logger::ptr(new mymuduo::Logger(mymuduo::LogLevel::ERROR, __FILE__, __LINE__, __func__))->stream()
    

pid_t gettid();

enum class LogLevel 
{
	DEBUG = 1,
	INFO = 2,
	WARN = 3,
	ERROR = 4,
    FITAL = 5 
};

class Logger
{
private:
    /* data */
    std::stringstream logStream_;
    LogLevel level_;
    std::string fileName_;
    int line_;
    std::string funcName_;

public:
    Logger(LogLevel level, const std::string& fileName, int line, const std::string& funcName);
    ~Logger();

public:
    using ptr = std::shared_ptr<Logger>;
    std::stringstream& stream();
    void log();
    static std::string getLogLevel(LogLevel& level);
    static const std::string getColorStr(LogLevel &level);
};

class AsyncLogger
{
public:
    using ptr = std::shared_ptr<AsyncLogger>;
    static AsyncLogger* getInstanceLogger();
    ~AsyncLogger();
private:
    AsyncLogger();
    void init();

public:
    void flush();
    void pushLog(const std::string& log);

private:
    std::mutex mutex_;
    sem_t semaphore_;
    std::condition_variable cond_;
    std::atomic_bool stop_ {false};
    std::shared_ptr<std::thread> thread_; //负责写日志的线程
    std::queue<std::string> task_;

private:
    std::string fileName_;
    std::string filePath_;
    int maxSize_;
    void writeLog();
    std::string date_;
    int index_{0};
    FILE* logFileHandle_{nullptr};
    static AsyncLogger* loggerPtr;
};

}
#endif // __LOGGER_H__