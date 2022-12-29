#include <mymuduo/tcpServer.h>
#include <mymuduo/Logger.h>
#include <string>
#include <functional>


class EchoServer
{
public:
    EchoServer(eventLoop *loop,
            const InetAdderss &addr,
            const std::string &name)
            :server_(loop, addr, name),
            loop_(loop)
            {
                server_.setConnectionCallback(
                    std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
                );

                server_.setMessageCallback(
                    std::bind(&EchoServer::onMessage, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
                );

                server_.setThreadNum(3);
            }
    void start()
    {
        server_.start();
    }
private:
    // 连接断开或建立的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LogInfo << "Connection up :" << conn->peerAddress().toIpPort();
        }
        else 
        {
            LogInfo << "Connection down :" << conn->peerAddress().toIpPort();
        }
    }

    //可读写事件的回调
    void onMessage(const TcpConnectionPtr &conn,
                Buffer *buf,
                timeStamp time)
    {   
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        //conn->shutdown(); // 写段关闭， epollHUP => closeback
    }
    tcpServer server_;
    eventLoop *loop_;

};

int main()
{
    eventLoop loop;
    InetAdderss addr(8000);
    EchoServer server(&loop, addr, "Echo-Server01");
    server.start();
    loop.loop();
    return 0;
}