#include <stdlib.h>
#include "poller.h"
#include "epollPoller.h"

poller *poller::newDefaultPoller(eventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;
    }
    else
    {
        return new epollPoller(loop);
    }
}