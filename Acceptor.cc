#include <unistd.h>
#include "Acceptor.h"
#include "Logger.h"
#include <errno.h>

static int createNonblocking();

Acceptor::Acceptor(eventLoop *loop, const InetAdderss &listenAddr, bool reuseport)
    : loop_(loop)
    , listenning_(false)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop_, acceptSocket_.fd())   
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCB(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::setNewConnectCallback(const NewConnectCallback &cb)
{
    newConnectCallback_ = cb;
}

bool Acceptor::listenning() const
{
    return listenning_;
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.socketListen();
    acceptChannel_.enableReading(); //向epoll中注册可读事件
}

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LogFatal << "listen sockfd created err:" << errno;
    }
    return sockfd;
}

// listenfd有事件发生，有新用户连接到来了
void Acceptor::handleRead()
{
    InetAdderss peerAddr;
    int connfd = acceptSocket_.socketAccept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectCallback_)
        {
            newConnectCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        if (errno == EMFILE)
        {
            ::close(connfd);
            LogFatal << "Accept sockfd reach limit...";
        }

        LogFatal << "Accept error: " << errno;
    }
}