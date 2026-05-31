#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>


using namespace muduo;
using namespace placeholders;

//获取单例对象
ChatService* ChatService::Instance()
{
    //单例模式
    static ChatService instance;
    return &instance;
}

//注册消息以及对应的业务handler
ChatService::ChatService()
{
    //注册消息以及对应的业务handler，将msgid作为key，将业务handler作为value，后续就可以通过msgid获取对应的业务handler
    //bind的作用是绑定成员函数到this指针
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChatMsg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChatMsg,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGOUT_MSG,std::bind(&ChatService::logout,this,_1,_2,_3)});


    //连接redis
    if(_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }

}


MsgHandler ChatService::getHandler(int msgId)
{

    //判断msgid是否存在
    if(_msgHandlerMap.find(msgId) == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgId << " not found handler!";
        };
    }
    else
    {
        //根据msgid获取对应的业务handler
        return _msgHandlerMap[msgId];
    }
    
    
 
}

//重置用户状态
void ChatService::rest()
{
    //将所有用户状态重置为offline
    _userModel.restState();
}



//处理登录业务
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if(user.getId() == id && user.getPassword() == pwd)
    {
        if(user.getState() == "online")
        {
            //用户已登录
            LOG_ERROR << "User " << id << " is online!";
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "User is online!";
            conn->send(response.dump());
        }else
        {
            //记录用户连接信息,这里记住这种加锁的技巧，不需要手动释放锁，用{}括起来
            //因为lock_guard是析构函数，会在{}结束时自动调用，释放锁
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }

            //订阅用户id的通道
            _redis.subscribe(id);

            //登录成功 ,更新用户状态为online
            user.setState("online");
            _userModel.updateState(user);
            
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
           
            //登录成功后，检查是否有离线消息
            vector<string> msgs = _offlineMsgModel.query(id);
            if(!msgs.empty())
            {
                //有离线消息，发送离线消息
                response["offlinemessages"] = msgs;
                _offlineMsgModel.remove(id);
            }

            //登录成功后，检查是否有好友并返回
            vector<User> friends = _friendModel.query(id);
            if(!friends.empty())
            {
                //有好友，发送好友
                vector<string> vec2;
                for(auto &user : friends)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }



             conn->send(response.dump());
        }

    }
    else
    {
        //登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "Login failed!";
        conn->send(response.dump());
    }
}
//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPassword(pwd);
    if(_userModel.insert(user))
    {
        LOG_INFO << "Reg success!";
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        LOG_ERROR << "Reg failed!";
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}
//处理客户端断开连接异常

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    //从_userConnMap中删除用户连接信息
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                //map中删除用户连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    //取消订阅用户id的通道
    _redis.unsubscribe(user.getId());

    //更新用户状态为offline
    if(user.getId() != 0)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

//处理退出业务
void ChatService::logout(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    //取消订阅用户id的通道
    _redis.unsubscribe(userid);

    //更新用户状态为offline
    User user;
    user.setId(userid);
    user.setState("offline");
    _userModel.updateState(user);
}


//处理添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    _friendModel.insert(userid,friendid);
    
}

//处理创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];
    Group group;
    group.setName(groupname);
    group.setDesc(groupdesc);
    if(_groupModel.createGroup(group)){
        //存储群组创建人信息
        _groupModel.addGroup(userid,group.getId(),"Creator");
    }
}


//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}


//处理群聊业务
void ChatService::groupChatMsg(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    string msg = js["msg"];

    //查询群组成员
    vector<int> useridVec = _groupModel.queryGroupUsers(userid,groupid);

    lock_guard<mutex> lock(_connMutex);
    for(int id: useridVec)
    {
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end())
        {
            //群组成员在线，发送消息
            it->second->send(js.dump());
        }
        else
        {
            // 发送消息到 toId 的通道。这里是当toid不在本机登陆，而在别的地方登陆时，需要将消息发送到toid的通道
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id,js.dump());
                
            }else
            {
                //群组成员不在线，存储离线消息
                _offlineMsgModel.insert(id,js.dump());
            }
        }
    }
}

// 一对一聊天业务
void ChatService::oneChatMsg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 需要接收信息的用户ID
    int toId = js["toid"].get<int>();
    
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toId);
        // 确认是在线状态
        if (it != _userConnMap.end())
        {
            // TcpConnection::send() 直接发送消息
            it->second->send(js.dump());
            return;
        }
    }

    // 发送消息到 toId 的通道。这里是当toid不在本机登陆，而在别的地方登陆时，需要将消息发送到toid的通道
    User user = _userModel.query(toId);
    if(user.getState() == "online")
    {
        _redis.publish(toId,js.dump());
        return;
    }
    
    // toId 不在线则存储离线消息
    _offlineMsgModel.insert(toId, js.dump());
}

//从redis消息队列获取消息
void ChatService::handleRedisSubscribeMessage(int channel, string message)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(channel);
    if(it != _userConnMap.end())
    {
        it->second->send(message);
        return;
    }
    // 未找到对应的用户连接信息，存储离线消息
    _offlineMsgModel.insert(channel, message);

}