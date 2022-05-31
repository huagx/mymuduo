#pragma once

#include <functional>
#include <memory>
#include "noncopyable.h"
#include "timeStamp.h"

class eventLoop;

class channel : noncopyable
{
public:
    channel(eventLoop *loop, int fd);
    ~channel();

    using eventCallback = std::function<void()> ;
    using readEventCallback = std::function<void(timeStamp)> ;

    //fd得到poller通知后，处理事件。
    void handleEvent(timeStamp receiveTime);
    
    //设置回调函数
    void setReadCB(readEventCallback cb);
    void setWriteCB(eventCallback cb);
    void setCloseCB(eventCallback cb);
    void setErrorCB(eventCallback cb);

    //防止当前channel被手动remove掉， channel还在执行回调操作。
    void tie(const std::shared_ptr<void>&);

    int fd() const;

    int events() const;

    //设置fd相应的事件状态
    void enableReading();
    void disableReading();
    void enableWriteing();
    void disableWriteing();
    void disableAll();

    //返回当前fd的事件状态
    bool isNoneEvent() const;
    bool isWriting() const;
    bool isReading() const;

    //
    int index() const;
    void setIndex(int index);

    void setRevents(int);

    eventLoop* ownerLoop() const;

    void remove();
private:
    void update();
    void handleEventWithGuard(timeStamp receiveTime);
    static const int KNoneEvent;
    static const int KReadEvent;
    static const int KWriteEvent;

    eventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;
    bool tied_;
    std::weak_ptr<void> tie_;

    //channel 通道里面能够获取和fd最终发生的具体事件revents， 他负责调用具体的事件回调操作。
    readEventCallback readCb_;
    eventCallback writeCb_;
    eventCallback closeCb_;
    eventCallback errorCb_;
};
