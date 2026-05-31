#include "chatserver.hpp"
#include <iostream>
#include <signal.h>
#include "chatservice.hpp"
using namespace std;

//处理服务器ctrl+c信号结束后，重置user状态
void resetHandler(int)
{
    ChatService::Instance()->rest();
    exit(0);
}

int main(int argc, char* argv[])
{
    // 注册ctrl+c信号信号处理函数
    signal(SIGINT,resetHandler);

    EventLoop loop;
    //从外部参数接受ip和端口
    if(argc != 3)
    {
        cout << "Usage: ./ChatServer ip port" << endl;
        return 1;
    }
    string ip = argv[1];
    int port = atoi(argv[2]);
    
    InetAddress listenAddr(ip, port);
    ChatServer server(&loop, listenAddr, "ChatServer");

    server.start();
    loop.loop();
    return 0;
}
