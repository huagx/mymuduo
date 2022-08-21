#include <functional>
#include <iostream>
#include "TcpConnection.h"
#include "eventLoop.h"
#include "logger.h"
#include "Socket.h"
#include "channel.h"
#include "eventLoop.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>

static eventLoop *checkLoopNotNull(eventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection loop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(eventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAdderss &localAddr,
                             const InetAdderss &peerAddr)
    : loop_(checkLoopNotNull(loop))
    , name_(name)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024)
{
    channel_->setReadCB(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCB(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCB(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCB(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd =%d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::handleRead(timeStamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead\n");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readAbleBytes() == 0)
            {
                channel_->disableWriteing();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }

                if (state_ == kDisconnecting)
                {
                    shutdownInloop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    if (connectionCallback_)
    {
        connectionCallback_(connPtr);
    }

    if (closeCallback_)
    {
        closeCallback_(connPtr);
    }
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optlen;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInloop, this));
    }
}

void TcpConnection::shutdownInloop()
{
    if (!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    if (connectionCallback_)
    {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectDestrory()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); //删除掉所有注册的事件
        if (connectionCallback_)
        {
            connectionCallback_(shared_from_this());
        }
    }
    channel_->remove();
}

/**
 * @brief 发送数据， 应用写的快， 内核发送数据慢， 需要把待发送数据写入缓冲区，而且设置了高水位。
 *
 * @param message
 * @param len
 */
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing \n");
        return;
    }

    if (!channel_->isWriting() && outputBuffer_.readAbleBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                //数据全部发送完成， 调用用户注册的回调， 也不用注册当前channel的写事件
                loop_->queueInLoop(std::bind(&TcpConnection::writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }
    
    if (!faultError && remaining > 0) //说明当前这一次没有把数据全部发送出去， 剩余的数据需要保存到缓冲区，然后给chennel_注册可写事件。
    {
        size_t oldLen = outputBuffer_.readAbleBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(TcpConnection::highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char *)data + nwrote, remaining);

        if (!channel_->isWriting())
        {
            channel_->enableWriteing(); //注册channel的写事件
        }
    }
}