#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>

#include "Socket.h"
#include "unistd.h"
#include "Logger.h"

Socket::Socket(int fd) : fd_(fd)
{
    
}

Socket::~Socket() 
{
    close(fd_);
}

int Socket::fd() const 
{
    return fd_;
}

void Socket::bindAddress(const InetAdderss &localAddr) 
{
    if (0 != ::bind(fd_, (sockaddr*)localAddr.getSockAddr(), sizeof(sockaddr_in))) 
    {
        LogFatal << "Bind socketFd " <<  fd_ <<" failed !";
    }
}

void Socket::socketListen()
{
    if (0 != ::listen(fd_, 1024)) 
    {
        LogFatal << "Listen socketFd " << fd_ << " failed!";
    }
}

int Socket::socketAccept(InetAdderss *peerAddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr, sizeof addr);
    int connfd = ::accept(fd_, (sockaddr*)&addr, &len);
    if (connfd >= 0) 
    {
        peerAddr->setSockAddr(addr);
    } 
    else 
    {
        return -1;
    }

    return connfd;
 }

void Socket::shutdownWrite() 
{
    if (::shutdown(fd_, SHUT_WR) < 0) 
    {
        LogError << "Socket shutdown Write error!";
    }    
}

void Socket::setTcpNoDelay(bool on) 
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on) 
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
} 

void Socket::setKeepAlive(bool on) 
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}

sockaddr_in Socket::getLocalAddr(int fd)
{
    sockaddr_in localAddr;
    bzero(&localAddr, sizeof localAddr);
    socklen_t addrLen = static_cast<socklen_t>(sizeof localAddr);
    if (::getsockname(fd, (sockaddr *)(&localAddr), &addrLen) < 0)
    {
        LogError << "Socket getLocalAddr error!";
    }

    return localAddr;
}