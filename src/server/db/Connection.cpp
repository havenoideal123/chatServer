#include "Connection.h"


Connection::Connection()
{
    // 初始化mysql连接
    _conn = mysql_init(nullptr);
}

Connection::~Connection()
{
    if(_conn != nullptr)
    {
        // 关闭mysql连接
        mysql_close(_conn);
    }
}

//连接数据库
bool Connection::connect(string server,int port,string user,string password,string dbname)
{
    // 连接数据库,mysql_real_connect()函数用于连接mysql数据库
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
bool Connection::update(string sql)
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

MYSQL_RES* Connection::query(string sql)
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

//转义sql语句中的特殊字符，防止sql注入攻击
string Connection::escapeString(const string &str)
{
    char *buf = new char[str.size() * 2 + 1];

    mysql_real_escape_string(
        _conn,
        buf,
        str.c_str(),
        str.size());

    string res(buf);
    delete[] buf;

    return res;
}


MYSQL* Connection::getConnection()
{
    // 返回mysql连接对象
    return _conn;
}