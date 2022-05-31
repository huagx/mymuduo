#pragma once

/**
 *noncopyable被继承以后，派生类对象不能进行拷贝构造和复制操作。 
 */
class noncopyable 
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

