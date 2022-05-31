#pragma once

#include "noncopyable.h"
#include "InetAdderss.h"

class InetAddress;
class Socket : noncopyable
{
public:
    explicit Socket(int fd);
    ~Socket();
    int fd() const;
    void bindAddress(const InetAdderss &localaddr);
    void socketListen();
    int socketAccept(InetAdderss *peerAddr);

    void shutdownWrite();
    void setReusePort(bool on);
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setKeepAlive(bool on);
    static sockaddr_in getLocalAddr(int fd);
private:
    const int fd_;
};

