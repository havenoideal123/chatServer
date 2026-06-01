# 基于Muduo的分布式聊天系统

## 项目简介

这是一个基于Muduo网络库开发的分布式聊天系统，支持用户注册、登录、一对一聊天、群组聊天、好友管理等功能。系统采用C/S架构设计，使用MySQL进行数据持久化，Redis提供高速缓存和消息订阅发布功能。

## 本人优化内容

本项目在原有 Muduo 分布式聊天系统基础上进行了二次开发与工程化优化，重点改进了数据库访问层的性能、稳定性和可维护性。

### 1. 自研 MySQL 数据库连接池

原项目中，每次数据库操作都会创建一个 MySQL 连接：

```cpp
MySQL mysql;

if(mysql.connect())
{
    mysql.update(sql);
}
```

这种方式在高并发场景下会频繁创建和销毁数据库连接，带来额外的性能开销。

为解决该问题，本项目实现了线程安全的 MySQL 数据库连接池：

- 使用单例模式管理全局唯一连接池实例
- 启动时预创建一定数量的数据库连接
- 当空闲连接不足时，由生产者线程动态创建新连接
- 通过扫描线程定期回收超时空闲连接
- 使用 shared_ptr 自定义删除器实现连接自动归还连接池
- 使用 mutex、atomic、condition_variable 保证多线程安全
- 支持连接获取超时机制，避免业务线程长时间阻塞

优化后，Model 层通过连接池复用数据库连接：

```cpp
auto conn = ConnectionPool::getInstance()->getConnection();

if(conn)
{
    conn->update(sql);
}
```

### 2. 重构 Model 层数据库访问方式

对 UserModel、FriendModel、GroupModel、OfflineMsgModel 等数据访问模块进行了统一改造：

- 将原来的“每次操作都重新连接数据库”改为“从连接池获取连接”
- 抽取 `dbpool_helper.hpp` 统一封装连接获取逻辑
- 降低 Model 层重复代码
- 提升数据库访问层的可维护性

### 3. 完善数据库连接生命周期管理

连接池内部通过 RAII 思想管理数据库连接生命周期：

- 业务线程获取连接后无需手动释放
- shared_ptr 析构时自动将连接归还到连接池
- 归还连接时刷新空闲时间
- 扫描线程根据最大空闲时间释放多余连接

### 4. 增强并发稳定性

连接池内部采用生产者-消费者模型：

- 消费者线程：业务层调用 `getConnection()` 获取连接
- 生产者线程：连接不足时创建新连接
- 扫描线程：定期清理空闲连接
- 条件变量：负责线程之间的等待与唤醒

该优化减少了数据库连接创建开销，提高了服务器在并发场景下的稳定性。

### 5. 数据库安全性增强

针对原项目中大量使用 `sprintf` 拼接 SQL 的情况，对数据库访问层进行了安全性增强。

原始写法示例：

```cpp
sprintf(sql,
        "insert into User(name,password,state) values('%s','%s','%s')",
        user.getName().c_str(),
        user.getPassword().c_str(),
        user.getState().c_str());
```

该方式如果直接拼接用户输入，可能出现以下问题：

- 用户输入中包含单引号时导致 SQL 语句结构被破坏
- 特殊字符可能引发 SQL 执行异常
- 存在 SQL 注入风险

优化后，在连接封装层提供字符串转义能力，对用户输入进行统一处理：

```cpp
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
```

Model 层在拼接字符串类型字段前，先进行转义处理：

```cpp
auto conn = getMysqlConn();

string name = conn->escapeString(user.getName());
string password = conn->escapeString(user.getPassword());
string state = conn->escapeString(user.getState());
```

主要改进：

- 对用户输入字符串进行统一转义
- 使用 `mysql_real_escape_string` 处理特殊字符
- 避免单引号等字符破坏 SQL 语句结构
- 降低 SQL 注入风险
- 提升数据库访问层稳定性
- 为后续改造成 MySQL 预处理语句预留扩展空间


## 功能特性

- **用户管理**：支持用户注册、登录、登出
- **好友系统**：添加好友、删除好友
- **消息系统**：
  - 一对一实时聊天
  - 群组聊天
  - 离线消息存储与推送
- **群组功能**：创建群组、加入群组、群组聊天
- **高可用性**：基于Redis的消息订阅发布机制
- **并发处理**：多线程安全设计

## 技术栈

- **网络库**：Muduo（高性能C++网络库，Reactor模型）
- **数据库**：MySQL（用户数据、消息记录存储）
- **数据库连接池**：自研 ConnectionPool（单例模式、生产者消费者模型、RAII）
- **数据库安全**：mysql_real_escape_string 字符串转义，降低 SQL 注入风险
- **缓存/消息中间件**：Redis（会话管理、消息订阅发布）
- **序列化**：JSON（数据传输格式）
- **并发编程**：mutex、atomic、condition_variable
- **编译构建**：CMake

## 系统架构

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   客户端 Client │────│ 服务端 Server    │────│  Redis Cluster  │
│                 │    │                  │    │                 │
│ - 登录/注册     │    │ - 用户认证       │    │ - 消息订阅发布  │
│ - 聊天界面      │    │ - 消息路由       │    │ - 在线状态管理  │
│ - 消息发送接收  │    │ - 业务逻辑处理   │    │ - 缓存优化      │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │
                              │
                    ┌──────────────────┐
                    │   Model 数据层   │
                    │ - UserModel      │
                    │ - FriendModel    │
                    │ - GroupModel     │
                    │ - OfflineMsgModel│
                    └──────────────────┘
                              │
                              │
                    ┌──────────────────┐
                    │ ConnectionPool   │
                    │                  │
                    │ - 连接复用       │
                    │ - 动态扩容       │
                    │ - 空闲回收       │
                    │ - 线程安全       │
                    └──────────────────┘
                              │
                              │
                    ┌──────────────────┐
                    │ MySQL Database   │
                    │                  │
                    │ - 用户信息       │
                    │ - 好友关系       │
                    │ - 群组信息       │
                    │ - 离线消息       │
                    └──────────────────┘
```

## 项目结构

```
chat_muduo/
├── chatServer/              # 服务端代码
│   ├── bin/                 # 可执行文件输出目录
│   ├── build/               # 构建相关
│   │   └── compile.md       # 编译说明
│   ├── include/             # 头文件
│   │   ├── server/          # 服务端核心头文件
│   │   │   ├── db/          # 数据库操作与连接池相关
│   │   │   │   ├── db.h
│   │   │   │   ├── CommonConnectionPool.h
│   │   │   │   └── dbpool_helper.hpp
│   │   │   ├── model/       # 数据模型
│   │   │   ├── redis/       # Redis操作相关
│   │   │   ├── chatserver.hpp    # 聊天服务器主类
│   │   │   └── chatservice.hpp   # 聊天业务处理类
│   │   ├── json.hpp         # JSON解析库
│   │   └── public.hpp       # 公共定义
│   ├── src/                 # 源代码
│   │   ├── server/          # 服务端源码
│   │   │   ├── main.cpp     # 服务端入口
│   │   │   ├── chatserver.cpp
│   │   │   ├── chatservice.cpp
│   │   │   ├── db/          # 数据库操作与连接池实现
│   │   │   │   ├── db.cpp
│   │   │   │   └── CommonConnectionPool.cpp
│   │   │   ├── model/       # 数据模型实现
│   │   │   └── redis/       # Redis操作实现
│   │   └── client/          # 客户端源码（示例）
│   └── CMakeLists.txt       # 构建配置
```

## 环境依赖

### 必需组件

- **操作系统**：Linux (推荐Ubuntu 18.04+ 或 CentOS 7+)
- **编译器**：GCC 7.0+
- **CMake**：3.0+
- **Muduo库**：muduo_net, muduo_base
- **MySQL**：8.0 或 5.7
- **Redis**：5.0+

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake libmuduo-dev libmysqlclient-dev libhiredis-dev

# CentOS/RHEL
sudo yum install gcc gcc-c++ cmake make muduo-devel mysql-devel hiredis-devel
```

## 编译部署

### 1. 克隆项目

```bash
git clone <repository-url>
cd chat_muduo/chatServer
```

### 2. 编译服务端

```bash
mkdir build && cd build
cmake ..
make
```

### 3. 配置数据库

在MySQL中创建数据库和表结构：

```sql
CREATE DATABASE chat_system CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE chat_system;

-- 用户表
CREATE TABLE user (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(50) NOT NULL,
    state ENUM('online', 'offline') DEFAULT 'offline'
);

-- 好友关系表
CREATE TABLE friend (
    userid INT NOT NULL,
    friendid INT NOT NULL,
    PRIMARY KEY (userid, friendid)
);

-- 群组表
CREATE TABLE allgroup (
    id INT AUTO_INCREMENT PRIMARY KEY,
    groupname VARCHAR(50) NOT NULL,
    groupdesc TEXT
);

-- 用户群组关系表
CREATE TABLE groupuser (
    groupid INT NOT NULL,
    userid INT NOT NULL,
    grole ENUM('creator', 'normal') DEFAULT 'normal',
    PRIMARY KEY (groupid, userid)
);

-- 离线消息表
CREATE TABLE offlinemessage (
    userid INT NOT NULL,
    message TEXT NOT NULL,
    PRIMARY KEY (userid)
);
```

### 4. 配置MySQL连接池

在服务端运行目录下配置 `mysql.conf`：

```conf
ip=127.0.0.1
port=3306
username=root
password=123456
dbname=chat_system
initSize=10
maxSize=1024
maxIdleTime=60
connectionTimeout=100
```

参数说明：

| 参数 | 说明 |
|------|------|
| ip | MySQL服务器IP地址 |
| port | MySQL服务器端口 |
| username | MySQL用户名 |
| password | MySQL密码 |
| dbname | 数据库名称 |
| initSize | 连接池初始连接数 |
| maxSize | 连接池最大连接数 |
| maxIdleTime | 连接最大空闲时间，单位为秒 |
| connectionTimeout | 获取连接超时时间，单位为毫秒 |

### 5. 启动Redis服务

```bash
# 启动Redis服务
redis-server /path/to/redis.conf

# 验证Redis是否正常运行
redis-cli ping  # 应该返回PONG
```

### 6. 启动聊天服务器

```bash
# 进入bin目录
cd ../bin

# 启动服务器 (IP地址和端口号可自定义)
./server 127.0.0.1 8080
```

## 使用说明

### 服务端启动参数

```bash
./server <IP地址> <端口号>
```

例如：
```bash
./server 0.0.0.0 8080  # 监听所有网卡的8080端口
```

### 客户端连接

客户端可以通过TCP协议连接到服务器指定端口，使用JSON格式进行通信。

#### 支持的消息类型

| 消息ID | 消息类型 | 参数 | 描述 |
|--------|----------|------|------|
| 1 | 注册 | `{"name":"用户名","password":"密码"}` | 用户注册 |
| 2 | 登录 | `{"id":用户ID,"password":"密码"}` | 用户登录 |
| 3 | 一对一聊天 | `{"toid":接收者ID,"msg":"消息内容"}` | 发送私聊消息 |
| 4 | 添加好友 | `{"userid":自己的ID,"friendid":好友ID}` | 添加好友 |
| 5 | 创建群组 | `{"userid":创建者ID,"groupname":"群名","groupdesc":"描述"}` | 创建群组 |
| 6 | 加入群组 | `{"userid":用户ID,"groupid":群ID}` | 加入群组 |
| 7 | 群聊 | `{"groupid":群ID,"msg":"消息内容"}` | 发送群聊消息 |
| 8 | 登出 | `{"id":用户ID}` | 用户登出 |

### 示例消息

注册消息：
```json
{
  "msgid": 1,
  "name": "张三",
  "password": "123456"
}
```

登录消息：
```json
{
  "msgid": 2,
  "id": 1001,
  "password": "123456"
}
```

一对一聊天：
```json
{
  "msgid": 3,
  "toid": 1002,
  "msg": "你好，我是张三！",
  "id": 1001
}
```

## 项目亮点

### 自研MySQL数据库连接池

本项目在原数据库访问方式基础上实现了自研数据库连接池，避免每次数据库操作都重新建立连接。

核心能力：

- 单例模式保证全局唯一连接池
- 生产者线程负责连接池动态扩容
- 扫描线程负责空闲连接回收
- shared_ptr 自定义删除器实现连接自动归还
- condition_variable 实现连接生产与消费之间的线程通信
- atomic 记录当前连接总数，辅助控制最大连接数量

### Model层统一接入连接池

原项目中各个 Model 类直接创建 MySQL 对象并连接数据库，代码重复且性能较低。

优化后，各个 Model 统一通过连接池获取连接：

```cpp
auto conn = getMysqlConn();

if(conn)
{
    conn->update(sql);
}
```

这样降低了重复代码，也使数据库连接管理集中到 ConnectionPool 中。

### 数据库安全增强

针对原项目中直接拼接 SQL 的问题，增加数据库输入字符串转义处理：

- 封装 `escapeString()` 方法统一处理字符串转义
- 对用户名、密码、消息内容、群名、群描述等字符串字段进行安全处理
- 避免特殊字符破坏 SQL 语句结构
- 降低 SQL 注入风险
- 提升数据库操作的健壮性

### 分布式消息转发

系统基于 Redis 发布订阅机制实现跨服务器消息转发，支持用户连接到不同服务器时仍然可以正常通信。

### 高并发网络模型

服务端基于 Muduo Reactor 模型实现 TCP 通信，能够较好支持多客户端并发连接。

## 系统特点

### 高性能设计

- 基于Reactor模式的Muduo网络库，支持高并发
- 异步非阻塞I/O模型
- Redis缓存加速数据访问
- 引入MySQL数据库连接池，减少频繁连接数据库的开销
- 通过连接复用、动态扩容、空闲回收提升数据库访问效率

### 数据安全

- 用户密码存储与身份认证
- 对用户输入字符串进行转义处理
- 使用 `mysql_real_escape_string` 处理特殊字符
- 避免单引号等字符破坏 SQL 语句结构
- 降低 SQL 注入风险
- 统一数据库连接获取接口
- 连接池集中管理数据库连接资源

### 可靠性保证

- 异常连接处理
- 服务重启后用户状态恢复
- 离线消息存储与推送
- 数据库连接获取超时处理
- 空闲连接自动回收，避免连接资源长期占用

## 开发说明

### 业务流程

1. **用户注册**：验证用户名唯一性，保存用户信息到数据库
2. **用户登录**：验证身份，更新在线状态，同步离线消息
3. **消息处理**：根据消息类型分发到对应处理器
4. **连接管理**：维护用户连接映射，处理异常断线

### 关键类说明

- `ChatServer`：网络层服务器，处理TCP连接和数据收发
- `ChatService`：业务层处理，包含所有聊天业务逻辑
- `UserModel`：用户数据模型，封装用户相关的数据库操作
- `FriendModel`：好友关系数据模型，封装好友增删查逻辑
- `GroupModel`：群组数据模型，封装创建群组、加入群组、查询群组等逻辑
- `OfflineMsgModel`：离线消息数据模型，封装离线消息存储、查询和删除逻辑
- `ConnectionPool`：自研MySQL数据库连接池，负责连接创建、分配、回收和线程同步
- `Connection`：MySQL连接封装，提供数据库连接、查询、更新等接口
- `Redis`：Redis操作封装，用于消息订阅发布

## 数据库连接池设计说明

### 设计目标

原始数据库访问方式中，每次执行SQL都需要重新创建MySQL连接。数据库连接建立过程涉及网络通信和认证过程，频繁创建和销毁会影响系统性能。

连接池的目标是：

- 提前创建数据库连接
- 复用已有连接
- 控制最大连接数量
- 自动回收空闲连接
- 保证多线程访问安全

### 核心流程

1. 服务器启动时，连接池读取 `mysql.conf` 配置文件。
2. 根据 `initSize` 创建初始数据库连接。
3. 业务线程需要访问数据库时，通过 `getConnection()` 获取连接。
4. 如果连接队列为空，业务线程会通过 `condition_variable` 等待。
5. 生产者线程在连接不足时创建新连接。
6. 业务线程使用完成后，shared_ptr 自定义删除器会自动将连接归还连接池。
7. 扫描线程定期检查空闲连接，超过最大空闲时间后释放多余连接。

### 改造前后对比

改造前：

```cpp
MySQL mysql;

if(mysql.connect())
{
    mysql.update(sql);
}
```

改造后：

```cpp
auto conn = getMysqlConn();

if(conn)
{
    conn->update(sql);
}
```

改造后，数据库连接由连接池统一管理，Model 层只负责业务SQL执行。

## SQL注入风险处理说明

### 问题背景

原项目中大量使用 `sprintf` 拼接 SQL：

```cpp
sprintf(sql,
        "insert into OfflineMessage(userid,message) values(%d,'%s')",
        userid,
        msg.c_str());
```

如果 `msg` 中包含单引号或恶意 SQL 片段，可能导致 SQL 语句结构被破坏，甚至引发 SQL 注入风险。

### 当前改进方案

本项目在 `Connection` 类中封装字符串转义函数：

```cpp
string escapeString(const string &str);
```

在 Model 层执行 SQL 前，对所有字符串类型字段进行转义，例如：

- 用户名 `name`
- 密码 `password`
- 用户状态 `state`
- 离线消息 `message`
- 群组名称 `groupname`
- 群组描述 `groupdesc`
- 群成员角色 `grole`

整数类型字段，例如 `userid`、`friendid`、`groupid`，不需要进行字符串转义。

### 改造原则

凡是 SQL 中通过 `%s` 拼接字符串字段的位置，都需要先调用：

```cpp
conn->escapeString(str);
```

再放入 SQL 语句中。

改造前：

```cpp
sprintf(sql,
        "insert into OfflineMessage(userid,message) values(%d,'%s')",
        userid,
        msg.c_str());
```

改造后：

```cpp
auto conn = getMysqlConn();

string safeMsg = conn->escapeString(msg);

sprintf(sql,
        "insert into OfflineMessage(userid,message) values(%d,'%s')",
        userid,
        safeMsg.c_str());
```

该方案可以减少 SQL 拼接带来的安全风险，并提升数据库访问层的健壮性。

## 故障排除

### 常见问题

1. **连接被拒绝**：检查服务器是否已启动，防火墙设置
2. **数据库连接失败**：确认MySQL服务运行，连接参数正确
3. **Redis连接失败**：确认Redis服务运行，网络可达
4. **编译错误**：确认所有依赖库已安装

### 日志查看

服务器运行时会在控制台输出相关信息，便于调试和问题定位。

## 扩展建议

- 添加消息加密功能
- 实现负载均衡和集群部署
- 增加文件传输功能
- 支持移动端应用
- 添加消息撤回功能
- 后续可将 SQL 拼接方式进一步升级为 MySQL 预处理语句，实现更严格的参数化查询

## 许可证

本项目仅供学习交流使用。

## 贡献

欢迎提交Issue和Pull Request来改进项目。