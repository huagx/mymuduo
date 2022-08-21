#include "tcpServer.h"
#include "TcpConnection.h"
#include "logger.h"

#include <functional>

static eventLoop* checkLoopNotNull(eventLoop* loop)
{
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null \n", __FILE__, __FUNCTION__, __LINE__);        
    }
    return loop;
}

tcpServer::tcpServer(eventLoop* loop, const InetAdderss &listenAddr, const std::string &nameArg, Option option)
    : loop_(checkLoopNotNull(loop)) 
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , accepter_(new Acceptor(loop, listenAddr, option == KReusePort))
    , threadPool_(new EventLoopThreadPool(loop, name_))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1)
{
    //当有新用户连接时， 会执行该回调。
    accepter_->setNewConnectCallback(std::bind(&tcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
    /**
     * 根据轮旋算法选择一个subloop
     * 唤醒subloop
     * 把当前connfd封装成channel分给subloop
     */
}

tcpServer::~tcpServer()
{
    for (auto& item : connections_) 
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestrory, conn));
    }    
}

//有新连接会执行回调
void tcpServer::newConnection(int sockfd, const InetAdderss &peerAddr)
{
    eventLoop *ioLoop = threadPool_->getNextLoop(); //轮询算法选择一个subloop,来管理channnel
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connectName = name_ + buf;
    LOG_INFO("tcpServer::newConnection [%s] - new connection [%s] from %s \n",
             name_.c_str(), connectName.c_str(), peerAddr.toIpPort().c_str());
    InetAdderss localAddr(Socket::getLocalAddr(sockfd));

    TcpConnectionPtr conn(new TcpConnection(ioLoop, connectName, sockfd, localAddr, peerAddr));
    connections_[connectName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&tcpServer::removeConnection, this, std::placeholders::_1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void tcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&tcpServer::removeConnectionInLoop, this, conn));    
}

void tcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("tcpServer::removeConnectionInLoop [%s], - connection %s\n", name_.c_str(), conn->name().c_str());

    size_t n = connections_.erase(conn->name());
    eventLoop* loop = conn->getLoop();
    loop->queueInLoop(std::bind(&TcpConnection::connectDestrory, conn));
}

void tcpServer::setThreadNum(int num) 
{
    threadPool_->setThreadNum(num);
}

//开启服务器监听
void tcpServer::start()
{
    if (started_++ == 0) //防止被启动多次 
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, accepter_.get())); 
    }
}