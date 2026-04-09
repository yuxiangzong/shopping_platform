#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "Protocol.h"

class Client
{
public:
    Client(const std::string &serverIp, int port);
    ~Client();

    json sendRequest(const json &request);
    void showMainMenu();
    void showMerchantMenu(const std::string &username);
    void showConsumerMenu(const std::string &username);

    static void printCommodities(const json &response);
    static void printCartItems(const json &response);
    static void printOrderItems(const json &order);

    void showCartMenu(const std::string &username);

private:
    void viewBalance(const json &request);
    void recharge(json &request);
    void changePassword(json &request);
    std::string _serverIp;
    int _port;
    int _sock{-1};
};

#endif
