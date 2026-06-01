#include "redis.hpp"
#include <iostream>
#include <thread>

using namespace std;

Redis::Redis()
: _publish_context(nullptr)
, _subscribe_context(nullptr)
{
}


Redis::~Redis()
{
  if(_publish_context)
  {
    //关闭发布上下文连接
    redisFree(_publish_context);
  }
  if(_subscribe_context)
  {
    //关闭订阅上下文连接
    redisFree(_subscribe_context);
  }
}

bool Redis::connect()
{
  //负责public发布消息的上下文连接
  _publish_context = redisConnect("127.0.0.1", 6379);
  if(_publish_context == nullptr)
  {
    cout << "connect redis failed" << endl;
    return false;
  }


  //负责subscribe消息的上下文连接
  _subscribe_context = redisConnect("127.0.0.1", 6379);
  if(_subscribe_context == nullptr)
  {
    cout << "connect redis failed" << endl;
    return false;
  }

  //在单独的线程中，监听通道上的事件，线程执行observer_channel_message函数，该函数会阻塞等待通道中的消息，有消息就给业务层上报
  thread t([&](){
    observer_channel_message();
  });
  t.detach(); //分离线程，不阻塞主线程
  //主线程继续执行，不阻塞等待observer_channel_message函数执行完毕
  cout << "connect redis success" << endl;
  return true;
}

//发布消息
bool Redis::publish(int channel, string message)
{
  //发布消息。PUBLISH 命令用于将消息发布到指定的通道上，每一个订阅该通道的客户端都会收到该消息，推送不会阻塞主线程，所以可以直接用redisCommand发送
  redisReply* reply = (redisReply*)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
  if(reply == nullptr)
  {
    cout << "publish message failed" << endl;
    return false;
  }
  //释放回复对象
  freeReplyObject(reply);
  return true;
  
}

//订阅消息
bool Redis::subscribe(int channel)
{
  //SUBSCRIBE 命令本身会造成线程阻塞等待通道中的消息，这里只做订阅通道，不接受消息
  //通道消息中的接收专门在observer_channel_message中处理
  //只发送命令，不阻塞接受响应，否则会和notifyMsg线程抢占资源
  //redisAppendCommand可以将命令添加到缓冲区中，不会阻塞主线程
  if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
  {
    cout << "subscribe channel failed" << endl;
    return false;
  }
  //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕
  int done= 0;
  while(!done)
  {
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕
    if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done))
    {
      cout << "subscribe channel failed" << endl;
      return false;
    }
  }
  return true;

}

//取消订阅消息
bool Redis::unsubscribe(int channel)
{
  //UNSUBSCRIBE 命令用于取消订阅指定的通道,同样适用redisAppendCommand
  if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
  {
    cout << "unsubscribe channel failed" << endl;
    return false;
  }
  //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕
  int done= 0;
  while(!done)
  {
    if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done))
    {
      cout << "unsubscribe channel failed" << endl;
      return false;
    }
  }
  return true;
}

//独立线程中接受订阅通道中的消息，这个阻塞的函数是单独在一个线程中运行的
void Redis::observer_channel_message()
{
  redisReply* reply = nullptr;
  //redisGetReply持续从订阅上下文连接中获取回复，直到返回REDIS_ERR
  while(REDIS_OK == redisGetReply(this->_subscribe_context, (void**)&reply))
  {
    //如果消息不为空
    if(reply !=nullptr && reply->element[2]!=nullptr&& reply->element[2]->str != nullptr)
    {
      //调用业务层的回调函数_notify_message_handler
      _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
    }
    //释放回复对象
    freeReplyObject(reply);
  }
  cerr << "observer_channel_message exit" << endl;
}

//回调函数注册器，将函数指针fn注册到_notify_message_handler成员变量中
void Redis::init_notify_handler(function<void(int,string)> fn)
{
  this->_notify_message_handler = fn;
}