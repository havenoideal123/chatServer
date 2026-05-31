#include "db.h"

MySQL::MySQL()
{
    // 初始化mysql连接
    _conn = mysql_init(nullptr);
}

MySQL::~MySQL()
{
    if(_conn != nullptr)
    {
        // 关闭mysql连接
        mysql_close(_conn);
    }
}

//连接数据库
bool MySQL::connect()
{
    // 连接数据库
    MYSQL *p = mysql_real_connect(_conn,server.c_str(),user.c_str(),password.c_str(),dbname.c_str(),3306,nullptr,0);
    if(p!=nullptr)
    {
        //c和c++默认编码ascii，不设置编码会导致中文乱码
        // 设置编码为gbk
        mysql_query(_conn,"set names gbk");
        LOG_INFO << "connect to database success";
    }
    return p;
}

//更新操作
bool MySQL::update(string sql)
{
    // 执行sql语句
    if(mysql_query(_conn,sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "update failed";
        return false;
    }
    return true;
}

//查询操作

MYSQL_RES* MySQL::query(string sql)
{
    // 执行sql语句
    if(mysql_query(_conn,sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "search failed";
        return nullptr;
    }
    // 返回查询结果
    return mysql_store_result(_conn);
}

MYSQL* MySQL::getConnection()
{
    // 返回mysql连接对象
    return _conn;
}