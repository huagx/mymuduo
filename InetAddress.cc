#include "InetAdderss.h"
#include <string.h>
#include <cstdio>

InetAdderss::InetAdderss(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    addr_.sin_port = htons(port);
}

std::string InetAdderss::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

std::string InetAdderss::toIpPort() const 
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    uint16_t port = toPort();
    sprintf(buf+strlen(buf), ":%u", port);
    return buf;
}

uint16_t InetAdderss::toPort() const {
    return ntohs(addr_.sin_port);
}

InetAdderss::~InetAdderss()
{
}

const sockaddr_in *InetAdderss::getSockAddr() const 
{
    return &addr_;
}

void InetAdderss::setSockAddr(const sockaddr_in &addr) 
{
    addr_ = addr;
}

// int main() 
// {
//     inetAdders addr(8080);
//     std::cout << addr.toIpPort() << std::endl;
// }
