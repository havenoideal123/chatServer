#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include <string>
using namespace std;


//数据库配置信息
static string server = "localhost";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

//数据库操作类
class MySQL
{
public:
    MySQL();
    ~MySQL();
    //连接数据库
    bool connect();
    //更新操作
    bool update(string sql);
    //查询操作
    MYSQL_RES* query(string sql);
    //获取插入的id
    MYSQL* getConnection();

private:
    MYSQL* _conn;
};



#endif
