# 基于Muduo的分布式聊天系统

## 项目简介

这是一个基于Muduo网络库开发的分布式聊天系统，支持用户注册、登录、一对一聊天、群组聊天、好友管理等功能。系统采用C/S架构设计，使用MySQL进行数据持久化，Redis提供高速缓存和消息订阅发布功能。

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

- **网络库**：Muduo（高性能C++网络库）
- **数据库**：MySQL（用户数据、消息记录存储）
- **缓存/消息中间件**：Redis（会话管理、消息订阅发布）
- **序列化**：JSON（数据传输格式）
- **编译构建**：CMake

## 系统架构

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   客户端 Client │────│ 服务端 Server    │────│  Redis Cluster  │
│                 │    │                  │    │                 │
│ - 登录/注册     │    │ - 用户认证       │    │ - 消息订阅发布  │
│ - 聊天界面      │    │ - 消息路由       │    │ - 在线状态管理  │
│ - 消息发送接收  │    │ - 业务逻辑处理   │    │ - 缓存优化     │
└─────────────────┘    └──────────────────┘    └─────────────────┘
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
│   │   │   ├── db/          # 数据库操作相关
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
│   │   │   ├── db/          # 数据库操作实现
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

### 4. 启动Redis服务

```bash
# 启动Redis服务
redis-server /path/to/redis.conf

# 验证Redis是否正常运行
redis-cli ping  # 应该返回PONG
```

### 5. 启动聊天服务器

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

## 系统特点

### 高性能设计

- 基于Reactor模式的Muduo网络库，支持高并发
- 异步非阻塞I/O模型
- Redis缓存加速数据访问

### 数据安全

- 用户密码加密存储
- SQL注入防护
- 连接池管理

### 可靠性保证

- 异常连接处理
- 服务重启后用户状态恢复
- 离线消息存储与推送

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
- `Redis`：Redis操作封装，用于消息订阅发布

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

## 许可证

本项目仅供学习交流使用。

## 贡献

欢迎提交Issue和Pull Request来改进项目。