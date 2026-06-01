#include "usermodel.hpp"
#include "dbpool_helper.hpp"
#include <iostream>
using namespace std;


// User表的增加方法
bool UserModel::insert(User &user)
{
  
    auto conn = getMysqlConn(); // 获取数据库连接

    
    char sql[1024] = {0};
    string name = conn->escapeString(user.getName());
    string password = conn->escapeString(user.getPassword());
    string state = conn->escapeString(user.getState());
    sprintf(sql,"insert into User(name,password,state) values('%s','%s','%s')",
            name.c_str(),password.c_str(),state.c_str());

    // 连接数据库
    if(conn)
    {
        // 执行sql语句
        if(conn->update(sql))
        {
            // 插入成功，返回true，并且将数据库生成的id赋值给user对象
            user.setId(mysql_insert_id(conn->getConnection()));
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
    auto conn = getMysqlConn();
    // 连接数据库
    if(conn)
    {
        // 执行sql语句
        MYSQL_RES *res = conn->query(sql);
        if(res!=nullptr)
        {
            // 查询成功，返回用户信息，mysql_fetch_row()函数返回的是一个MYSQL_ROW类型的指针，指向查询结果的第一行
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row!=nullptr)
            {
                // 转换为User对象
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                //mysql_free_result()函数用于释放查询结果集占用的内存空间
                mysql_free_result(res);
                return user;
            }
            mysql_free_result(res);
           
        }
    }
    return User();
}

bool UserModel::updateState(User &user)
{

    auto conn = getMysqlConn();

    // 构建sql语句
    char sql[1024] = {0};
    // 更新用户状态
    string state = conn->escapeString(user.getState());
    sprintf(sql,"update User set state = '%s' where id = %d",
            state.c_str(),user.getId());
            
    // 连接数据库
    if(conn)
    {
        // 执行sql语句
        if(conn->update(sql))
        {
            // 更新成功，返回true
            return true;
        }
    }
    return false;
}
     
void UserModel::resetState()
{
    // 构建sql语句
    char sql[1024] = {0};
    sprintf(sql,"update User set state = 'offline' where state = 'online'");
    auto conn = getMysqlConn();
    // 连接数据库
    if(conn)
    {
        // 执行sql语句
        conn->update(sql);
        // 更新成功
        return ;
    }
}