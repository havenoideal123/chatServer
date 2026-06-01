#pragma once
#include "Connection.h"
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
using namespace std;

//实现连接池模块
class ConnectionPool
{
public:
  //单例模式，获取连接池实例
  static ConnectionPool* getInstance();

  //给外部一个接口，从连接池获取一个可用的空闲连接,使用shared_ptr管理连接的生命周期，当shared_ptr析构时，自动关闭连接
  shared_ptr<Connection> getConnection();

private:

  ConnectionPool(); //单例，私有构造函数


  void  produceConnectionTask(); //生产者线程任务函数
  void scannerConnectionTask(); //扫描线程任务函数
  bool loadConfig(); //加载配置文件
  string _ip; //数据库服务器IP地址
  unsigned int _port;   //数据库服务器端口号
  string _username; //数据库用户名
  string _password; //数据库密码
  string _dbname; //数据库名称
  int _initSize; //连接池初始化大小
  int _maxSize; //连接池最大大小
  int _maxIdleTime; //连接池最大空闲时间
  int _connectionTimeout; //连接超时时间

  queue<Connection*> _connectionQueue; //连接池队列
  mutex _queueMutex; //维护连接池线程安全的互斥锁
  atomic_int _connectionCnt = 0; //记录连接所创建的connection连接的总数量

  condition_variable cv; //条件变量，用于连接生产线程和消费线程通信
};
