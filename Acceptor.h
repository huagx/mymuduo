#pragma once 

#include <functional>
#include "Socket.h"
#include "channel.h"

class eventLoop;
class InetAdderss;
class Acceptor
{
public:
    using NewConnectCallback = std::function<void(int socketfd, const InetAdderss&)>;
    Acceptor(eventLoop *loop, const InetAdderss& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectCallback(const NewConnectCallback &cb);
    bool listenning() const;
    void listen();
    void handleRead();
private:
    /* data */
    eventLoop *loop_;
    bool listenning_;
    Socket acceptSocket_;
    channel acceptChannel_;
    NewConnectCallback newConnectCallback_;
};

