#include "offlinemessagemodel.hpp"
#include "dbpool_helper.hpp"

void OfflineMsgModel::insert(int userid,string msg)
{

    auto conn = getMysqlConn(); // 获取数据库连接
    // 构建sql语句
    char sql[1024] = {0};
    // 插入离线消息
    string message = conn->escapeString(msg);
    sprintf(sql,"insert into OfflineMessage(userid,message) values(%d,'%s')",
            userid,message.c_str());
    // 连接数据库
    if(conn)
    {
        // 执行sql语句
        conn->update(sql);
        return;
    }
}

void OfflineMsgModel::remove(int userid)
{
    // 构建sql语句
    char sql[1024] = {0};
    // 删除离线消息
    sprintf(sql,"delete from OfflineMessage where userid = %d",userid);
    auto conn = getMysqlConn(); // 获取数据库连接
    // 连接数据库
    if(conn)
    {
        // 执行sql语句
        conn->update(sql);
        return;

    }
}

vector<string> OfflineMsgModel::query(int userid)
{
    // 构建sql语句
    char sql[1024] = {0};
    // 查询离线消息
    sprintf(sql,"select message from OfflineMessage where userid = %d",userid);
    auto conn = getMysqlConn(); // 获取数据库连接
    vector<string> vec;
    if(conn)
    {
        MYSQL_RES *res = conn->query(sql);
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