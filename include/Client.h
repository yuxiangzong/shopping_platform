#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

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
