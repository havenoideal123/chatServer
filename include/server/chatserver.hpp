#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

//聊天服务器的朱磊
class ChatServer
{
public:
    ChatServer(EventLoop* loop, //事件循环
        const InetAddress& listenAddr, //监听地址——ip:port
        const string& nameArg);
    
    //开启事件循环
    void start();
private:
    //专门处理用的连接创建和断开的回调函数，
    void onConnection(const TcpConnectionPtr& conn);

    //专门处理用的读写事件回调函数，
    void onMessage(const TcpConnectionPtr& conn, //连接
                            Buffer* buffer, //缓冲区
                            Timestamp time); //时间戳
    TcpServer _server; //组合的muduo库，实现服务器功能的类对象
    EventLoop* _loop; //指向事件循环对象的指针
};

#endif