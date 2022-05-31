#pragma once

#include <vector>
#include <unordered_map>
#include "noncopyable.h"
#include "timeStamp.h"

class channel;
class eventLoop;
//io复用模块
class poller : noncopyable 
{
public:
    poller(eventLoop* loop);
    virtual ~poller() = default;
    using channelList = std::vector<channel*>;
    virtual timeStamp poll(int timeouts, channelList *activeChannels) = 0;
    virtual void updateChannel(channel*) = 0;
    virtual void removeChannel(channel*) = 0;

    bool hasChannel(channel* channel) const;

    static poller* newDefaultPoller(eventLoop* loop);    
protected:
    using channelMap = std::unordered_map<int, channel*>;
    channelMap channels_;
private:
    eventLoop* ownerLoop_; 
};

