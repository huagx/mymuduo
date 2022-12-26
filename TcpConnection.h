#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "InetAdderss.h"
#include "noncopyable.h"
#include "CallBack.h"
#include "Buffer.h"
#include "timeStamp.h"

class channel;
class eventLoop;
class Socket;

/**
 * @brief TcpServer => Acceptor =>新用户的连接 
 * 通过accept拿到connfd => TcpConnection 设置回调 => channel => epoller => channel的回调函数
 * 
 */

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(eventLoop *loop,
                  const std::string &name,
                  int socket,
                  const InetAdderss &localAddr,
                  const InetAdderss &peerAddr);
    ~TcpConnection();

    eventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAdderss &localAddress() const { return localAddr_; }
    const InetAdderss &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    void send(const std::string &buf);

    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) { highWaterMarkCallback_ = cb; }

    void connectEstablished();
    void connectDestrory();

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    void setState(StateE state) { state_ = state; }
    void handleRead(timeStamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void shutdownInloop();
    void sendInLoop(const void *data, size_t len);
    eventLoop *loop_; //不是baseLoop, 因为Tcpconnection都是在subLoop中管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<channel> channel_;

    const InetAdderss localAddr_;
    const InetAdderss peerAddr_;

    ConnectionCallback connectionCallback_;       //有新连接的回调
    MessageCallback messageCallback_;             //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; //消息写完时的回调
    CloseCallback closeCallback_;                 //关闭连接时回调
    HighWaterMarkCallback highWaterMarkCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};
