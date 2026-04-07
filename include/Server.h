#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "Protocol.h"
#include "Database.h"
#include "User.h"
#include "Commodity.h"
#include "OrderManager.h"

class Server
{
public:
    Server(int port);
    ~Server();
    void start();

private:
    int _port;
    Database _db;
    UserManager _userManager;
    CommodityManager _commodityManager;
    OrderManager _orderManager;

    std::unordered_map<std::string, std::function<json(const json &)>> _handlers;

    // 线程池
    std::vector<std::thread> _workers;
    std::queue<std::function<void()>> _tasks;
    std::mutex _queueMutex;
    std::condition_variable _cv;
    std::atomic<bool> _stop{false};
    std::shared_mutex _dataMutex;

    void handleConnection(int socket);

    json handleLogin(const json &request);
    json handleRegister(const json &request);
    json handleSearchCommodities(const json &request);
    json handleBalanceOperation(User *user, int operation, const json &request);
    json handlePasswordChange(User *user, const json &request);
    json handleCartOperation(const json &request);
    json handleOrderOperation(const json &request);
    json handleMerchantOperation(const json &request);
    json handleConsumerOperation(const json &request);

    static json commodityToJson(const Commodity *commodity);
    bool isReadOnly(const std::string &action, const json &request) const;
};

#endif
