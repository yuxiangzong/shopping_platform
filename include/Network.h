#ifndef NETWORK_H
#define NETWORK_H

#include <functional>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "User.h"
#include "Order.h"
#include "Commodity.h"

using json = nlohmann::json;

// 服务器类
class Server
{
public:
    // 构造函数
    Server(int port);
    // 启动服务器
    void start();

private:
    int _port;                                                                    // 端口号
    std::unordered_map<std::string, std::function<json(const json &)>> _handlers; // 处理函数映射表
    UserManager _userManager;                                                     // 用户管理器
    CommodityManager _commodityManager;                                           // 商品管理器
    ShoppingCart _shoppingCart;                                                   // 购物车
    std::vector<Order *> _orders;                                                 // 订单列表

    // 处理登录请求
    json handleLogin(const json &request);
    // 处理注册请求
    json handleRegister(const json &request);
    // 处理商品搜索请求
    json handleSearchCommodities(const json &request);
    // 处理用户余额操作请求
    json handleBalanceOperation(User *user, int operation, const json &request) const;
    // 处理密码修改请求
    json handlePasswordChange(User *user, const json &request) const;
    // 处理购物车操作请求
    json handleCartOperation(const json &request);
    // 处理订单操作请求
    json handleOrderOperation(const json &request);
    // 处理商家操作请求
    json handleMerchantOperation(const json &request);
    // 处理消费者操作请求
    json handleConsumerOperation(const json &request);
};

// 客户端类
class Client
{
public:
    // 构造函数
    Client(const std::string &serverIp, int port);
    // 向服务器发送请求并接收响应
    json sendRequest(const json &request) const;
    // 显示主菜单
    void showMainMenu() const;
    // 显示商家菜单
    void showMerchantMenu(const std::string &username) const;
    // 显示消费者菜单
    void showConsumerMenu(const std::string &username) const;

private:
    std::string _serverIp; // 服务器IP地址
    int _port;             // 服务器端口号
};

#endif
