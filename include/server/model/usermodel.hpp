#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"

class UserModel
{
public:
    //插入用户信息
    bool insert(User &user);
    //根据用户id查询用户信息
    User query(int id);
    //更新用户状态
    bool updateState(User &user);
    //重置用户状态为offline
    void resetState();
};


#endif