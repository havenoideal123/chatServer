#include "CommonConnectionPool.h"
#include <fstream>
#include <iostream>

using namespace std;
//线程安全的懒汉单例模式实现,什么是懒汉：只有第一次调用的时候才会创建实例
ConnectionPool* ConnectionPool::getInstance()
{
    static ConnectionPool instance;
    return &instance;
}

bool ConnectionPool::loadConfig() //加载配置文件
{
  FILE* fp = fopen("./mysql.conf", "r");
  if (!fp)
  {
    std::cout << "load config file failed" << endl;
    return false;
  }
  char line[1024] = {0};
  //读取配置文件
  while(fgets(line,1024,fp) != nullptr)
  {
    string str = line;
    //查找=，返回序号
    int idx = str.find("=",0);
    if(idx ==-1)
    {
      continue;
    }
    //查找换行符，返回序号
    int endidx = str.find("\n",idx);
    string key = str.substr(0,idx);
    //提取配置项的值，去掉首尾空格
    string value = str.substr(idx+1,endidx-idx-1);
  
    if(key == "ip")
    {
      _ip = value;
    }
    else if(key == "port")
    {
      _port = atoi(value.c_str());
    }
    else if(key == "username")
    {
      _username = value;
    }
    else if(key == "password")
    {
      _password = value;
    }
    else if(key == "dbname")
    {
      _dbname = value;
    }
    else if(key == "initSize")
    {
      _initSize = atoi(value.c_str());
    }
    else if(key == "maxSize")
    {
      _maxSize = atoi(value.c_str());
    }
    else if(key == "maxIdleTime")
    { 
      _maxIdleTime = atoi(value.c_str());
    }
    else if(key == "connectionTimeout")
    {
      _connectionTimeout = atoi(value.c_str());
    }

  }

  fclose(fp);
  return true;
}

//连接池构造函数
ConnectionPool::ConnectionPool()
{
  if(!this->loadConfig())
  {
    LOG_INFO << "load config file failed";
    return;
  }
  //初始化连接池队列，根据initSize创建对应数量连接池
  for(int i=0;i<_initSize;i++)
  {
    Connection* conn = new Connection();
    //连接数据库
    if(!conn->connect(_ip,_port,_username,_password,_dbname))
    {
      conn->refreshAliveTime(); //刷新起始空闲时间，每创建一个连接，就刷新一次起始空闲时间，记录连接创建的时间，这样可以计算连接的存活时间，判断是否超过最大空闲时间
      //将连接放入空闲队列
      _connectionQueue.push(conn);
      //增加连接池连接数量
      _connectionCnt++;
    }
    else
    {
      delete conn;
    }
  }

  //启动一个线程作为生产者，名字为produce，处理函数为produceConnectionTask
  thread produce(std::bind(&ConnectionPool::produceConnectionTask,this));
  produce.detach(); //分离线程，将produce线程从主线程分离，主线程继续执行

  //启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，对于连接的回收
  thread scanner(std::bind(&ConnectionPool::scannerConnectionTask,this));
  scanner.detach(); //分离线程，将scanner线程从主线程分离，主线程继续执行
}

void ConnectionPool::produceConnectionTask() //生产者线程任务函数

{
  //保持运行
  for(;;)
  {
    unique_lock<mutex> lock(_queueMutex); //获取互斥锁，确保线程安全，因为连接池队列是共享资源，需要保护起来
    while (!_connectionQueue.empty())
    {
      //队列不为空，此处生产线程进入等待
      cv.wait(lock); 
    }
    if(_connectionCnt < _maxSize)
    {
      //创建新的连接
      Connection *p = new Connection();
      p->connect(_ip,_port,_username,_password,_dbname);
      p->refreshAliveTime(); //刷新起始空闲时间
      _connectionQueue.push(p);
      _connectionCnt++;

    }
    cv.notify_all(); //通知消费线程可以消费连接


  }
}

shared_ptr<Connection> ConnectionPool::getConnection()
{
  //获取互斥锁，确保线程安全，因为连接池队列是共享资源，需要保护起来
  unique_lock<mutex> lock(_queueMutex);
  
  //如果连接池队列为空，等待连接生产线程生产连接
  while(_connectionQueue.empty())
  {

    if(cv_status::timeout == cv.wait_for(lock,chrono::milliseconds(_connectionTimeout)))
    {
      if(_connectionQueue.empty())
      {
        LOG_INFO << "get connection timeout";
        return nullptr;
      
      }
    }

  }
  /*
  shared_ptr管理连接的生命周期，当shared_ptr析构时，自动关闭连接,会调用Connection的析构函数
  这里需要自定义shared_ptr的析构函数，把connction归还队列
  */
  shared_ptr<Connection> conn(_connectionQueue.front(),
    [&](Connection* pcon){
      unique_lock<mutex> lock(_queueMutex);
      pcon->refreshAliveTime(); //刷新起始空闲时间
      _connectionQueue.push(pcon);
      cv.notify_all(); //通知生产者线程可以生产连接
    });
  _connectionQueue.pop();
  cv.notify_all(); //通知生产者线程可以生产连接
  return conn;
}

void ConnectionPool::scannerConnectionTask() //扫描线程任务函数
{
  //保持运行
  for(;;)
  {
    //通过sleep模拟睡眠效果，每隔maxIdleTime时间扫描一次队列，释放多余连接
    this_thread::sleep_for(chrono::seconds(_maxIdleTime));

    //扫描整个队列，释放多余连接
    unique_lock<mutex> lock(_queueMutex);
    while(_connectionCnt > _initSize && !_connectionQueue.empty())
    {
      //检查队列头连接是否超过最大空闲时间，如果超过，释放连接
      Connection * p = _connectionQueue.front();
      if(p->getAliveTime() >= (_maxIdleTime*1000))
      {
        _connectionQueue.pop();

        delete p; //释放连接
        _connectionCnt--; //减少连接池连接数量
      }
      else
      {
        break; //队列头连接没有超过最大空闲时间，后续连接更不会超过，直接退出扫描
      }
    }
  }
}