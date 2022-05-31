#pragma once 

#include <vector>
#include <string>

class Buffer
{
public:
    static const std::size_t kCheapPrepend = 8;
    static const std::size_t kInitialSize = 1024;
    explicit Buffer(std::size_t initialSize = kInitialSize);

    size_t readAbleBytes() const;
    size_t writeAbleBytes() const;
    size_t prependAbleBytes() const;
    const char* peek() const; //返回缓冲区中可读数据的起始地址
    void retrieve(size_t len); //
    void retrieveAll();
    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t len);
    void ensureWriteableBytes(size_t len);
    const char* beginWrite() const;
    char* beginWrite();
    void append(const char* data, size_t len);
    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);
    ~Buffer();
private:
    void makeSpace(size_t len);
    char* begin(); //获取vector的首元素的地址。
    const char* begin() const;
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

};
