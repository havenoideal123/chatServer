#ifndef PUBLIC_H
#define PUBLIC_H

//公共头文件
enum EnMsgType
{
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK, // 登录消息确认
    REG_MSG, // 注册响应信息
    REG_MSG_ACK, // 注册消息确认
    ONE_CHAT_MSG, // 一对一聊天消息
    ADD_FRIEND_MSG, // 添加好友消息
    CREATE_GROUP_MSG, // 创建群组消息
    ADD_GROUP_MSG, // 加入群组消息
    GROUP_CHAT_MSG, // 群组聊天消息
    LOGOUT_MSG, // 退出消息
};


#endif
