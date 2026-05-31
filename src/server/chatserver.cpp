#include "chatserver.hpp"
#include "json.hpp"
#include  <functional>
#include <string>
#include "chatservice.hpp"
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop, //事件循环
        const InetAddress& listenAddr, //监听地址——ip:port
        const string& nameArg) //服务器名称
        : _server(loop, listenAddr, nameArg), _loop(loop)
{
    //固定写法
    //设置连接回调
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    //设置读写事件回调
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
    //开启事件循环
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    //开启事件循环
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    //客户端断开连接
    if(!conn->connected())
    {
        ChatService::Instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, //连接
                            Buffer* buffer, //缓冲区
                            Timestamp time) //时间戳
{
    string buf = buffer->retrieveAllAsString();
    //数据的反序列化
    json js = json::parse(buf);
    //完全解耦网络模块的代码和业务模块的代码
    //通过js["msgid"]获取-》业务handler-》
    auto msgHandler =  ChatService::Instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器来处理业务
    msgHandler(conn,js,time);
}
