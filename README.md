# 购物平台 (Shopping Platform)

基于 C/S 架构的终端购物平台，使用 C++17 开发，服务端采用 epoll I/O 多路复用 + 线程池处理并发，SQLite3 持久化存储数据。

## 架构概览

```
┌──────────────┐                               ┌──────────────────────────┐
│              │     TCP (长度前缀 + JSON)      │                          │
│  终端客户端   │ ◄──────────────────────────► │  服务端                   │
│  (交互式菜单) │                               │  epoll + 线程池           │
│              │                               │  内存缓存 + SQLite3 (WAL) │
└──────────────┘                               └──────────────────────────┘
```

- **通信协议**：4 字节大端长度前缀成帧，JSON 负载
- **并发模型**：epoll 水平触发 + `EPOLLONESHOT` + 线程池（线程数 = CPU 核心数）
- **数据存储**：SQLite3，关键操作使用事务保证原子性
- **缓存策略**：按需加载，先查内存缓存，未命中再查数据库，读写锁保护

## 功能

### 商家
- 商品管理：添加、修改价格/库存/折扣
- 批量折扣：按商品类型一键设置折扣
- 查看余额、充值、修改密码

### 消费者
- 浏览/搜索商品
- 购物车：添加、删除、修改数量、结账
- 订单管理：查看、支付、取消
- 未支付订单 30 分钟自动取消

### 用户系统
- 注册（商家/消费者）、登录
- 密码加盐哈希存储

## 项目结构

```
├── CMakeLists.txt           # 构建配置
├── include/
│   ├── nlohmann/json.hpp    # JSON 库 (header-only)
│   ├── Protocol.h           # 网络协议：成帧、收发、操作枚举
│   ├── Database.h           # SQLite 封装
│   ├── User.h               # 用户体系 + UserManager
│   ├── Commodity.h          # 商品 + CommodityManager
│   ├── Order.h              # 订单数据类
│   ├── OrderManager.h       # 购物车与订单管理
│   ├── Server.h             # 服务端类
│   └── Client.h             # 客户端类
└── src/
    ├── server_main.cpp      # 服务端入口
    ├── client_main.cpp      # 客户端入口
    ├── Server.cpp           # 服务端实现
    ├── Client.cpp           # 客户端实现
    ├── Database.cpp         # 数据库初始化与建表
    ├── User.cpp             # 用户管理实现
    ├── Commodity.cpp        # 商品管理实现
    └── OrderManager.cpp     # 购物车/订单/结账/支付逻辑
```

## 依赖

- C++17 编译器 (GCC 8+ / Clang 7+)
- CMake 3.14+
- SQLite3 开发库 (`libsqlite3-dev` / `sqlite-devel`)

## 构建与运行

```bash
# 构建
mkdir -p build && cd build
cmake ..
make

# 启动服务端（参数：端口号）
./shopping_server 8080

# 启动客户端（参数：服务端 IP、端口号）
./shopping_client 127.0.0.1 8080
```

服务端启动后会在当前目录创建 `shopping.db` 数据库文件。

## 许可证

本项目采用 [MIT 许可证](LICENSE)。nlohmann/json 库同样遵循 MIT 许可证。