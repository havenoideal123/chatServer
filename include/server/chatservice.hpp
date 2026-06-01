#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include "json.hpp"
#include "usermodel.hpp"
#include <mutex>
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"


using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
//只要符合这个函数签名的函数都可以作为MsgHandler
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,json &js,Timestamp time)>;


//聊天服务器业务类，即网络层接受客户端请求，业务层处理请求，网络层返回响应
class ChatService
{
public:
    //单例模式，整个程序中只创建一个ChatService对象
    //因为业务处理类通常不需要创建多个对象。所有客户端登录、注册、聊天消息，都可以统一交给同一个 ChatService 对象处理。
    static ChatService* Instance();

    //处理登录业务
    void login(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理一对一聊天业务
    void oneChatMsg(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理添加好友业务
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理创建群组业务
    void createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理加入群组业务
    void addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理群聊业务
    void groupChatMsg(const TcpConnectionPtr &conn,json &js,Timestamp time);


    //获取消息对应的处理器
    MsgHandler getHandler(int msgId);

    //处理客户端断开连接异常
    void clientCloseException(const TcpConnectionPtr &conn);

    //处理退出业务
    void logout(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理redis订阅消息
    void handleRedisSubscribeMessage(int channel,string msg);


    //重置
    void reset();
private:
    //构造函数私有化，防止外部创建对象
    ChatService();

    //将msgid和业务handler绑定起来
    unordered_map<int,MsgHandler> _msgHandlerMap;

    //存储在线用的连接信息
    unordered_map<int,TcpConnectionPtr> _userConnMap;

    //互斥锁，用于保护_userConnMap线程安全
    mutex _connMutex;

    //数据操作类对象
    UserModel _userModel;

    //离线消息数据操作类对象
    OfflineMsgModel _offlineMsgModel;

    //好友数据操作类对象
    FriendModel _friendModel;

    //群组数据操作类对象
    GroupModel _groupModel;

    //redis对象
    Redis _redis;
};

#endif
