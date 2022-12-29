#include "Logger.h"
#include <sys/time.h>
#include <assert.h>
#include <functional>

namespace mymuduo
{
    static thread_local pid_t t_thread_id = 0;
    pid_t gettid()
    {
        if (t_thread_id == 0)
        {
            t_thread_id = syscall(SYS_gettid);
        }
        return t_thread_id;
    }

    AsyncLogger* AsyncLogger::loggerPtr = new AsyncLogger();
    std::stringstream &Logger::stream()
    {
        timeval timeval;
        gettimeofday(&timeval, nullptr);

        struct tm time;
        localtime_r(&(timeval.tv_sec), &time);

        const char *format = "%Y-%m-%d %H:%M:%S";
        char buf[128];
        strftime(buf, sizeof(buf), format, &time);

        logStream_ << getColorStr(level_);

        logStream_ << "[" << buf << "." << timeval.tv_usec << "]\t";

        std::string s_level = getLogLevel(level_);
        logStream_ << "[" << s_level << "]\t";

        logStream_ << "[" << gettid() << "]\t"
                   << "[" << fileName_ << ":" << line_ << "]\t";
        // << "[" << m_func_name << "]\t";
        return logStream_;
    }

    std::string Logger::getLogLevel(LogLevel &level)
    {
        std::string re = "DEBUG";
        switch (level)
        {
        case LogLevel::DEBUG:
            re = "DEBUG";
            return re;

        case LogLevel::INFO:
            re = "INFO";
            return re;

        case LogLevel::WARN:
            re = "WARN";
            return re;

        case LogLevel::ERROR:
            re = "ERROR";
            return re;

        case LogLevel::FITAL:
            re = "FITAL";

        default:
            return re;
        }
        return re;
    }

    
    const std::string Logger::getColorStr(LogLevel &level)
    {
        switch (level)
        {
        case LogLevel::DEBUG:
            return "\033[37m";
        case LogLevel::INFO:
            return "\033[32m";
        case LogLevel::WARN:
            return "\033[33m";
        case LogLevel::ERROR:
            return "\033[31m";
        case LogLevel::FITAL:
            return "\033[35m";
        default:
            return "\033[37m";;
        }
    }

    Logger::Logger(LogLevel level, const std::string &fileName, int line, const std::string &funcName)
        : level_(level), fileName_(fileName), line_(line), funcName_(funcName)
    {
    }

    Logger::~Logger()
    {
        log();
        if (level_ == LogLevel::FITAL)
        {
            exit(-1);
        }
    }

    void Logger::log()
    {
        logStream_ << "\033[0m";
        logStream_ << "\n";
        AsyncLogger::getInstanceLogger()->pushLog(logStream_.str());
    }


    void AsyncLogger::writeLog()
    {
        int ret = sem_post(&semaphore_);
        assert(ret == 0);
        bool isReloadFileHandler = false;
        while(!stop_)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            while(task_.empty() && !stop_)
            {
                cond_.wait(lck);   //日志队列中为空时，一直wait，并且释放锁，让别的线程可以获取到锁，并在条件满足时通知它。
            }
            std::string curLog;
            curLog.swap(task_.front());
            task_.pop();
            lck.unlock();

            timeval now;
            gettimeofday(&now, nullptr);

            struct tm now_time;
            localtime_r(&(now.tv_sec), &now_time);

            const char *format = "%Y%m%d";
            char date[32];
            strftime(date, sizeof(date), format, &now_time);
            if (date_ != std::string(date))
            {
                index_ = 0;
                date_= std::string(date);
                isReloadFileHandler = true;
            }

            if (logFileHandle_ == nullptr)
            {
                isReloadFileHandler = true;
            }

            std::stringstream ss;
            ss << filePath_ << fileName_ << "_" << date_ << "_"  << index_ << ".log";
            std::string newLogFile = ss.str();

            if(isReloadFileHandler)
            {
                if (logFileHandle_)
                {
                    fclose(logFileHandle_);
                }

                logFileHandle_ = fopen(newLogFile.c_str(), "a");
                isReloadFileHandler = false;
            }

            if (ftell(logFileHandle_) > maxSize_)  //当前日志文件是否超过文件大小限制
            {
                fclose(logFileHandle_);

                // single log file over max size
                index_++;
                std::stringstream ss;
                ss << filePath_ << fileName_ << "_" << date_ << "_" << index_ << ".log";
                newLogFile = ss.str();

                logFileHandle_ = fopen(newLogFile.c_str(), "a");
                isReloadFileHandler = false;
            }

            assert(logFileHandle_ != nullptr);

            size_t nWrite = fwrite(curLog.c_str(), 1, curLog.size(), logFileHandle_);
            assert(nWrite == curLog.size());

            fflush(logFileHandle_);
        }

        if (logFileHandle_)
        {
            fclose(logFileHandle_);
        }
    }

    void AsyncLogger::flush()
    {
        if (logFileHandle_)
        {
            fflush(logFileHandle_);
        }
    }


    void AsyncLogger::pushLog(const std::string& log)
    {
        if (log.empty())
        {
            return;
        }
        {
            std::unique_lock<std::mutex> lck(mutex_);
            task_.push(std::move(log));
        }
        cond_.notify_one();
    }

    AsyncLogger* AsyncLogger::getInstanceLogger()
    {
       return loggerPtr;
    }

    void AsyncLogger::init()
    {
        int ret = sem_init(&semaphore_, 0, 0);
        assert(ret == 0);
        fileName_ = "mymuduo";
        filePath_ = "/tmp/";
        maxSize_ = 100*1024*1024;
        thread_ = std::make_shared<std::thread>(std::bind(&AsyncLogger::writeLog, this));
        sem_wait(&semaphore_);//确保写日志线程创建完成，并启动之后初始化函数结束
    }

    AsyncLogger::AsyncLogger()
    {
        init();
    }

    AsyncLogger::~AsyncLogger()
    {
        thread_->join();
    }
}