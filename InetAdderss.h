#pragma once

#include<netinet/in.h>
#include<arpa/inet.h>
#include<iostream>

//封装socket地址类型
class InetAdderss
{

public:
    explicit InetAdderss(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAdderss(const sockaddr_in &addr);
    ~InetAdderss();

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in *getSockAddr() const;
    void setSockAddr(const sockaddr_in &addr);
private:
    sockaddr_in addr_;
};

