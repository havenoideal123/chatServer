#include "chatserver.hpp"
#include <iostream>
#include <signal.h>
#include "chatservice.hpp"
using namespace std;

//处理服务器信号结束后，重置user状态
void resetHandler(int)
{
    ChatService::Instance()->reset();
    exit(0);
}

int main(int argc, char* argv[])
{
    // 注册SIGINT,SIGTERM,SIGABRT信号信号处理函数
    signal(SIGINT,resetHandler);
    signal(SIGTERM,resetHandler);
    signal(SIGABRT,resetHandler);


    
    //从外部参数接受ip和端口
    if(argc != 3)
    {
        cout << "Usage: ./ChatServer ip port" << endl;
        return 1;
    }
    string ip = argv[1];
    int port = atoi(argv[2]);
    
    InetAddress listenAddr(ip, port);

    EventLoop loop;
    ChatServer server(&loop, listenAddr, "ChatServer"); //创建服务器对象

    server.start(); //启动服务器
    loop.loop(); //进入事件循环，等待客户端连接
    return 0;
}
