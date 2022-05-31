#pragma once

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <unordered_map>
#include "eventLoop.h"
#include "eventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "InetAdderss.h"
#include "noncopyable.h"
#include "Acceptor.h"
#include "CallBack.h"

class tcpServer
{
public:
    using ThreadInitCallback = std::function<void(eventLoop*)>;
    enum Option
    {
        KnoReusePort,
        KReusePort,
    };
    tcpServer(eventLoop *loop, 
                const InetAdderss &listenAddr, 
                const std::string &nameArg,
                Option option = KnoReusePort);
    ~tcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb);
    void setConnectionCallback(const ConnectionCallback &cb);
    void setMessageCallback(const MessageCallback &cb);
    void setWriteCompleteCallback(const WriteCompleteCallback &cb);

    //设置subloop的个数
    void setThreadNum(int num);

    //开启服务器监听
    void start();
private:
    void newConnection(int sockfd, const InetAdderss &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);
    
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>; 
    eventLoop *loop_;
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> accepter_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;//有新连接的回调
    MessageCallback messageCallback_;//有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息写完时的回调

    ThreadInitCallback threadInitCallback_;
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_;
};
