#include "poller.h"
#include "eventLoop.h"
#include "channel.h"

poller::poller(eventLoop* loop) : ownerLoop_(loop)
{

}

bool poller::hasChannel(channel* chnl) const
{
    auto it = channels_.find(chnl->fd());
    return it != channels_.end() && it->second == chnl;
}


