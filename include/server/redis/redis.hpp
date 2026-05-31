#ifndef REDIS_H
#define REDIS_H

#include <functional>
#include <string>
#include <hiredis/hiredis.h>
using namespace std;



class Redis
{
public:
    Redis();
    ~Redis();


    //连接redis
    bool connect();

    //向redis指定的channel发送消息
    bool publish(int channel, string msg);

    //向redis指定的通道订阅消息
    bool subscribe(int channel);
    //取消redis指定的通道订阅消息
    bool unsubscribe(int channel);

    //在独立线程中接受订阅通道中的消息
    void observer_channel_message();

    //初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int,string)> fn);

private:
    //hiredis同步上下文对象，负责publish消息
    redisContext* _publish_context;
    //hiredis同步上下文对象，负责subscribe消息
    redisContext* _subscribe_context;

    //回调操作，接收订阅的消息，给service上报
    function<void(int,string)> _notify_message_handler; 

};


#endif
