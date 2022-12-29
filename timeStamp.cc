#include "timeStamp.h"
#include <cstdio>
#include <iostream>


timeStamp::timeStamp():microSecondsSinceEpoch_(0) 
{

}

timeStamp::timeStamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{

}

timeStamp timeStamp::now() 
{
    return timeStamp(time(NULL));
}

std::string timeStamp::toString () const 
{
    char buf[128] = { 0 };
    tm *tmTime = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", 
        tmTime->tm_year + 1900, 
        tmTime->tm_mon + 1,
        tmTime->tm_mday,
        tmTime->tm_hour,
        tmTime->tm_min,
        tmTime->tm_sec
    );

    return buf;
}


