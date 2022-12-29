#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include <algorithm>

#include "Buffer.h"

Buffer::Buffer(std::size_t initialSize) 
    : buffer_(kCheapPrepend + initialSize)
    , readerIndex_(kCheapPrepend)
    , writerIndex_(kCheapPrepend)
    {

    }

void Buffer::ensureWriteableBytes(size_t len) 
{
    if (writeAbleBytes() < len) 
    {
        makeSpace(len);
    }
}

void Buffer::makeSpace(size_t len) 
{
    if (writeAbleBytes() + prependAbleBytes() < len + kCheapPrepend) 
    {   
        buffer_.resize(writerIndex_ + len);
    }
    else 
    {
        size_t readAble = readAbleBytes();
        std::copy(begin() + readerIndex_,
                begin() + writerIndex_,
                begin() + kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readAble;
    }
}

char* Buffer::begin()
{
    return &*buffer_.begin();
}

const char* Buffer::begin() const
{
    return &*buffer_.begin();
}


const char* Buffer::beginWrite() const 
{
    return begin() + writerIndex_;
}


char* Buffer::beginWrite()
{
    return begin() + writerIndex_;
}

void Buffer::append(const char* data, size_t len) 
{
    ensureWriteableBytes(len);
    std::copy(data, data+len, beginWrite());
    writerIndex_ += len;
}

/**
 * @brief 从fd上读取数据， poller工作在LT模式, 读不完数据下次继续读。
 * 
 * @param fd 
 * @param saveErrno 
 */
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = {0};
    struct iovec vec[2];

    const size_t writeAble = writeAbleBytes();
    vec[0].iov_base = begin() + writerIndex_;  //这是buff底层缓冲区剩余的可写空间大小
    vec[0].iov_len = writeAble;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writeAble < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0) 
    {
        *saveErrno = errno;
    }
    else if (n <= static_cast<ssize_t>(writeAble)) //buffer可写缓冲区足够写下当前数据 
    {
        writerIndex_ += n;
    }
    else 
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writeAble);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno) 
{
    ssize_t n = ::write(fd, peek(), readAbleBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}

Buffer::~Buffer()
{
    
}

size_t Buffer::readAbleBytes() const
{
    return writerIndex_ - readerIndex_;
}

size_t Buffer::writeAbleBytes() const
{
    return buffer_.size() - writerIndex_;
}

size_t Buffer::prependAbleBytes() const 
{
    return readerIndex_;
}

const char* Buffer::peek() const
{
    return begin() + readerIndex_;
}

void Buffer::retrieve(size_t len) 
{
    if (len < readAbleBytes()) 
    {
        readerIndex_ += len; //应用只读取了可读缓冲区数据的一部分， len,
    }
    else 
    {
        retrieveAll();
    }
}

void Buffer::retrieveAll() 
{
    readerIndex_ = writerIndex_ = kCheapPrepend;  //重置可读可写位置
}

std::string Buffer::retrieveAsString(size_t len) 
{
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

std::string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readAbleBytes());
}




