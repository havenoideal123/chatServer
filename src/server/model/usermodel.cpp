#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;


// User表的增加方法
bool UserModel::insert(User &user)
{
    // 构建sql语句
    char sql[1024] = {0};
    // 插入用户信息
    sprintf(sql,"insert into User(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(),user.getPassword().c_str(),user.getState().c_str());
    MySQL mysql;
    // 连接数据库
    if(mysql.connect())
    {
        // 执行sql语句
        if(mysql.update(sql))
        {
            // 插入成功，返回插入的id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
        
}

User UserModel::query(int id)
{
    // 构建sql语句
    char sql[1024] = {0};
    sprintf(sql,"select * from User where id = %d",id);
    MySQL mysql;
    // 连接数据库
    if(mysql.connect())
    {
        // 执行sql语句
        MYSQL_RES *res = mysql.query(sql);
        if(res!=nullptr)
        {
            // 查询成功，返回用户信息
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row!=nullptr)
            {
                // 转换为User对象
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
           
        }
    }
    return User();
}

bool UserModel::updateState(User &user)
{
    // 构建sql语句
    char sql[1024] = {0};
    // 更新用户状态
    sprintf(sql,"update User set state = '%s' where id = %d",
            user.getState().c_str(),user.getId());
    MySQL mysql;
    // 连接数据库
    if(mysql.connect())
    {
        // 执行sql语句
        if(mysql.update(sql))
        {
            // 更新成功，返回true
            return true;
        }
    }
    return false;
}
     
void UserModel::restState()
{
    // 构建sql语句
    char sql[1024] = {0};
    sprintf(sql,"update User set state = 'offline' where state = 'online'");
    MySQL mysql;
    // 连接数据库
    if(mysql.connect())
    {
        // 执行sql语句
        if(mysql.update(sql))
        {
            // 更新成功
            return ;
        }
    }
}