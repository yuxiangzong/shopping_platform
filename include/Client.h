#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "Protocol.h"

class Client
{
public:
    Client(const std::string &serverIp, int port);

    json sendRequest(const json &request) const;
    void showMainMenu() const;
    void showMerchantMenu(const std::string &username) const;
    void showConsumerMenu(const std::string &username) const;

    static void printCommodities(const json &response);
    static void printCartItems(const json &response);
    static void printOrderItems(const json &order);

    void showCartMenu(const std::string &username) const;

private:
    std::string _serverIp; // 服务器IP地址
    int _port;             // 服务器端口号
};

#endif
