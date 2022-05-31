#pragma once
#include <iostream>
class timeStamp
{
public:
    timeStamp();
    explicit timeStamp(int64_t microSecondsSinceEpoch);
    static timeStamp now();
    int64_t microSecondsSinceEpoch_;
    std::string toString() const;
};
