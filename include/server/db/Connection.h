#ifndef CONNECTION_H
#define CONNECTION_H

#include <mysql/mysql.h>
#include <string>
#include <ctime>
#include <muduo/base/Logging.h>
using namespace std;



//数据库操作类
class Connection
{
public:
    Connection();
    ~Connection();
    //连接数据库
    bool connect(string server,int port,string user,string password,string dbname);
       //更新操作
    bool update(string sql);
    //查询操作
    MYSQL_RES* query(string sql);
    //获取连接
    MYSQL* getConnection();

    //转义sql语句中的特殊字符，防止sql注入攻击
    string escapeString(const string &str);
  
    //刷新连接存活时间
    void refreshAliveTime(){_aliveTime = clock();};
    //获取连接存活时间
    clock_t getAliveTime(){return clock() - _aliveTime;};


private:
    MYSQL* _conn;

    clock_t _aliveTime; //记录进入空闲状态后的起始存活时间
};


#endif
