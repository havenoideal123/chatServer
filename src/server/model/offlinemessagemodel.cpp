#include "offlinemessagemodel.hpp"
#include "db.h"

void OfflineMsgModel::insert(int userid,string msg)
{
    // 构建sql语句
    char sql[1024] = {0};
    // 插入离线消息
    sprintf(sql,"insert into OfflineMsg(userid,msg) values(%d,'%s')",
            userid,msg.c_str());
    MySQL mysql;
    // 连接数据库
    if(mysql.connect())
    {
        // 执行sql语句
        if(mysql.update(sql))
        {
            return;
            // 插入成功
        }
    }
}

void OfflineMsgModel::remove(int userid)
{
    // 构建sql语句
    char sql[1024] = {0};
    // 删除离线消息
    sprintf(sql,"delete from OfflineMsg where userid = %d",userid);
    MySQL mysql;
    // 连接数据库
    if(mysql.connect())
    {
        // 执行sql语句
        if(mysql.update(sql))
        {
            return;
            // 删除成功
        }
    }
}

vector<string> OfflineMsgModel::query(int userid)
{
    // 构建sql语句
    char sql[1024] = {0};
    // 查询离线消息
    sprintf(sql,"select message from OfflineMsg where userid = %d",userid);
    MySQL mysql;
    vector<string> vec;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}