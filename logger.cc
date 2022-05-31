#include <iostream>
#include "logger.h"
#include "timeStamp.h"

Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLoggerLevel(int level)
{
    logLevel_ = level;
}

void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    default:
        break;
    }

    std::cout << timeStamp::now().toString() << " : " << msg << std::endl;
}