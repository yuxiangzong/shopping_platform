#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include "Server.h"
#include "Order.h"

json Server::commodityToJson(const Commodity *commodity)
{
    return {{"name", commodity->getName()},
            {"type", commodity->getCommodityType()},
            {"description", commodity->getDescription()},
            {"merchant", commodity->getMerchant()},
            {"basePrice", commodity->getBasePrice()},
            {"price", commodity->getPrice()},
            {"discount", commodity->getDiscount()},
            {"storage", commodity->getStorage()}};
}

bool Server::isReadOnly(const std::string &action, const json &request) const
{
    if (action == "searchCommodities" || action == "login")
        return true;
    if (action == "register")
        return false;

    int operation = request.value("operation", -1);

    if (action == "merchantOperation")
        return operation == static_cast<int>(MerchantOp::ViewBalance) ||
               operation == static_cast<int>(MerchantOp::ListMyCommodities);

    if (action == "consumerOperation")
        return operation == static_cast<int>(ConsumerOp::ViewBalance) ||
               operation == static_cast<int>(ConsumerOp::ViewCommodities) ||
               operation == static_cast<int>(ConsumerOp::SearchCommodities);

    if (action == "cartOperation")
        return operation == static_cast<int>(CartOp::ViewCart);

    if (action == "orderOperation")
        return operation == static_cast<int>(OrderOp::ViewPending);

    return false; // 未知操作默认独占锁
}

Server::Server(int port) : _port(port)
{
    _handlers["login"] = [this](const json &req)
    { return handleLogin(req); };
    _handlers["register"] = [this](const json &req)
    { return handleRegister(req); };
    _handlers["searchCommodities"] = [this](const json &req)
    { return handleSearchCommodities(req); };
    _handlers["cartOperation"] = [this](const json &req)
    { return handleCartOperation(req); };
    _handlers["orderOperation"] = [this](const json &req)
    { return handleOrderOperation(req); };
    _handlers["merchantOperation"] = [this](const json &req)
    { return handleMerchantOperation(req); };
    _handlers["consumerOperation"] = [this](const json &req)
    { return handleConsumerOperation(req); };

    // 创建线程池
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 4;
    for (unsigned int i = 0; i < numThreads; ++i)
    {
        _workers.emplace_back([this]
                              {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->_queueMutex);
                    _cv.wait(lock, [this] {
                        return _stop || !_tasks.empty();
                    });
                    if (_stop && _tasks.empty()) {
                        return;
                    }
                    task = std::move(_tasks.front());
                    _tasks.pop();
                }
                task();
            } });
    }
    std::cout << "Thread pool started with " << numThreads << " workers." << '\n';
}

Server::~Server()
{
    _stop = true;
    _cv.notify_all();
    for (auto &worker : _workers)
    {
        if (worker.joinable())
            worker.join();
    }
}

void Server::handleConnection(int clientSocket)
{
    try
    {
        json request = recvJson(clientSocket);
        std::string action = request["action"];

        json response;
        bool readOnly = isReadOnly(action, request);

        if (readOnly)
        {
            // 读操作：共享锁，允许多个读者并发
            std::shared_lock<std::shared_mutex> lock(_dataMutex);
            auto it = _handlers.find(action);
            response = (it != _handlers.end())
                           ? it->second(request)
                           : json({{"status", "error"}, {"message", "Unknown action"}});
        }
        else
        {
            // 写操作：独占锁，阻塞其他读者和写者
            std::unique_lock<std::shared_mutex> lock(_dataMutex);
            auto it = _handlers.find(action);
            response = (it != _handlers.end())
                           ? it->second(request)
                           : json({{"status", "error"}, {"message", "Unknown action"}});

            // 仅在数据变更时保存
            if (_userManager.isDirty() || _commodityManager.isDirty())
            {
                if (_userManager.saveUsers() && _commodityManager.saveCommodities())
                {
                    std::cout << "Users and commodities saved successfully." << '\n';
                }
                else
                {
                    std::cout << "Failed to save users or commodities." << '\n';
                }
            }
        }

        sendJson(clientSocket, response);
    }
    catch (const std::exception &e)
    {
        try
        {
            sendJson(clientSocket, {{"status", "error"}, {"message", e.what()}});
        }
        catch (...)
        {
            // 客户端已断开，无法发送错误响应
        }
    }

    close(clientSocket);
}

void Server::start()
{
    int server_fd, new_socket;

    // 创建socket文件描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    // 绑定socket到端口
    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 开始监听
    // 忽略 SIGPIPE，防止客户端断开时 send 导致进程崩溃
    signal(SIGPIPE, SIG_IGN);

    if (listen(server_fd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port " << _port << '\n';

    while (true)
    {
        // 接受新连接
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }

        // 设置接收超时（30秒），防止客户端连接后不发请求导致线程池耗尽
        struct timeval timeout{.tv_sec = 30, .tv_usec = 0};
        setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // 将连接分发给线程池
        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _tasks.emplace([this, new_socket]()
                           { handleConnection(new_socket); });
        }
        _cv.notify_one();
    }
}

json Server::handleLogin(const json &request)
{
    std::cout << "Handling login..." << '\n';
    std::string username = request["username"];
    std::string password = request["password"];

    User *user = _userManager.login(username, password);

    if (user)
    {
        return {
            {"status", "success"},
            {"userType", user->getUserType()}};
    }
    else
    {
        return {{"status", "error"}, {"message", "Invalid username or password"}};
    }
}

json Server::handleRegister(const json &request)
{
    std::cout << "Handling register..." << '\n';
    std::string type = request["type"];
    std::string username = request["username"];
    std::string password = request["password"];

    bool success = _userManager.registerUser(type, username, password);

    if (success)
    {
        return {{"status", "success"}};
    }
    else
    {
        return {{"status", "error"}, {"message", "User already exists"}};
    }
}

json Server::handleBalanceOperation(User *user, int operation, const json &request)
{
    std::cout << "Handling balance operation..." << '\n';
    switch (operation)
    {
    case static_cast<int>(MerchantOp::ViewBalance): // 查看余额
        return {
            {"status", "success"},
            {"balance", user->getBalance()}};

    case static_cast<int>(MerchantOp::Recharge): // 充值
    {
        double amount = request["amount"];
        if (user->addBalance(amount))
        {
            _userManager.markDirty();
            return {
                {"status", "success"},
                {"balance", user->getBalance()}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Invalid amount"}};
        }
    }
    default:
        return {{"status", "error"}, {"message", "Invalid balance operation"}};
    }
}

json Server::handlePasswordChange(User *user, const json &request)
{
    std::cout << "Handling password change..." << '\n';
    std::string newPassword = request["newPassword"];
    if (user->setPassword(newPassword))
    {
        _userManager.markDirty();
        return {{"status", "success"}};
    }
    else
    {
        return {{"status", "error"}, {"message", "Invalid password"}};
    }
}

json Server::handleSearchCommodities(const json &request)
{
    std::cout << "Handling search commodities..." << '\n';
    std::string name = request.value("name", "");

    std::vector<const Commodity *> commodities = _commodityManager.findCommodity(name);

    json result = json::array();
    for (const Commodity *commodity : commodities)
    {
        result.push_back(commodityToJson(commodity));
    }

    return {{"status", "success"}, {"commodities", result}};
}

json Server::handleMerchantOperation(const json &request)
{
    std::cout << "Handling merchant operation..." << '\n';
    std::string username = request["username"];
    int operation = request["operation"];
    User *user = _userManager.getUserByUsername(username);

    if (!user || user->getUserType() != "Merchant")
    {
        return {{"status", "error"}, {"message", "Invalid merchant user"}};
    }

    switch (operation)
    {
    case static_cast<int>(MerchantOp::ViewBalance): // 查看余额
    case static_cast<int>(MerchantOp::Recharge):    // 充值
        return handleBalanceOperation(user, operation, request);

    case static_cast<int>(MerchantOp::AddCommodity): // 添加商品
    {
        std::string type = request["type"];
        std::string name = request["commodityName"];
        std::string description = request["description"];
        double price = request["price"];
        int stock = request["stock"];

        if (_commodityManager.addCommodity(type, name, username, description, price, stock))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to add commodity"}};
        }
    }

    case static_cast<int>(MerchantOp::ListMyCommodities): // 获取商家商品列表
    {
        auto commodities = _commodityManager.getMutableCommodityByMerchant(username);
        json result = json::array();
        for (const auto &commodity : commodities)
        {
            result.push_back(commodityToJson(commodity));
        }
        return {{"status", "success"}, {"commodities", result}};
    }
    case static_cast<int>(MerchantOp::ModifyPrice): // 修改价格
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        double newPrice = request["newPrice"];
        if (commodity->setBasePrice(newPrice))
        {
            _commodityManager.markDirty();
            return {{"status", "success"}, {"commodity", commodityToJson(commodity)}};
        }
        else
        {
            return {{"status", "error"}, {"message", "价格修改失败"}};
        }
    }
    case static_cast<int>(MerchantOp::ModifyStock): // 修改库存
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        int newStock = request["newStock"];
        if (commodity->setStorage(newStock))
        {
            _commodityManager.markDirty();
            return {{"status", "success"}, {"commodity", commodityToJson(commodity)}};
        }
        else
        {
            return {{"status", "error"}, {"message", "库存修改失败"}};
        }
    }
    case static_cast<int>(MerchantOp::ModifyDiscount): // 修改单个商品折扣
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        double newDiscount = request["newDiscount"];
        if (commodity->setDiscount(newDiscount))
        {
            _commodityManager.markDirty();
            return {{"status", "success"}, {"commodity", commodityToJson(commodity)}};
        }
        else
        {
            return {{"status", "error"}, {"message", "折扣修改失败"}};
        }
    }

    case static_cast<int>(MerchantOp::BatchModifyDiscount): // 批量修改同类型商品折扣
    {
        std::string commodityType = request["commodityType"];
        double newDiscount = request["newDiscount"];
        auto commodities = _commodityManager.getMutableCommodityByMerchant(username);
        int modifiedCount = 0;

        for (auto &commodity : commodities)
        {
            if (commodity->getCommodityType() == commodityType)
            {
                if (commodity->setDiscount(newDiscount))
                {
                    modifiedCount++;
                }
            }
        }

        if (modifiedCount > 0)
        {
            _commodityManager.markDirty();
            return {
                {"status", "success"},
                {"message", "成功修改" + std::to_string(modifiedCount) + "个商品的折扣"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "没有找到匹配类型的商品或折扣修改失败"}};
        }
    }

    case static_cast<int>(MerchantOp::ChangePassword): // 修改密码
        return handlePasswordChange(user, request);

    default:
        return {{"status", "error"}, {"message", "Invalid operation"}};
    }
}

json Server::handleConsumerOperation(const json &request)
{
    std::cout << "Handling consumer operation" << '\n';
    std::string username = request["username"];
    int operation = request["operation"];
    User *user = _userManager.getUserByUsername(username);

    if (!user || user->getUserType() != "Consumer")
    {
        return {{"status", "error"}, {"message", "Invalid consumer user"}};
    }

    switch (operation)
    {
    case static_cast<int>(ConsumerOp::ViewBalance): // 查看余额
    case static_cast<int>(ConsumerOp::Recharge):    // 充值
        return handleBalanceOperation(user, operation, request);

    case static_cast<int>(ConsumerOp::ViewCommodities): // 查看商品
        return handleSearchCommodities({{"action", "searchCommodities"}});

    case static_cast<int>(ConsumerOp::SearchCommodities): // 搜索商品
    {
        std::string name = request.value("name", "");
        return handleSearchCommodities({{"action", "searchCommodities"}, {"name", name}});
    }

    case static_cast<int>(ConsumerOp::ChangePassword): // 修改密码
        return handlePasswordChange(user, request);

    case static_cast<int>(ConsumerOp::CartOperation): // 购物车操作
        return handleCartOperation(request);

    case static_cast<int>(ConsumerOp::OrderOperation): // 待支付订单
        return handleOrderOperation(request);

    default:
        return {{"status", "error"}, {"message", "Invalid operation"}};
    }
}

json Server::handleCartOperation(const json &request)
{
    std::cout << "Handling cart operation" << '\n';
    std::string username = request["username"];
    int operation = request["operation"];
    User *user = _userManager.getUserByUsername(username);

    if (!user || user->getUserType() != "Consumer")
    {
        return {{"status", "error"}, {"message", "Invalid consumer user"}};
    }

    Consumer *consumer = dynamic_cast<Consumer *>(user);

    switch (operation)
    {
    case static_cast<int>(CartOp::ViewCart): // 查看购物车
    {
        auto items = consumer->getShoppingCart().getItems();
        json result = json::array();
        for (const auto &item : items)
        {
            result.push_back({{"name", item.first->getName()},
                              {"quantity", item.second},
                              {"price", item.first->getPrice()},
                              {"subtotal", item.first->getPrice() * item.second}});
        }
        return {{"status", "success"}, {"items", result}, {"total", consumer->getShoppingCart().calculateTotal()}};
    }

    case static_cast<int>(CartOp::AddItem): // 添加商品到购物车
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity)
        {
            return {{"status", "error"}, {"message", "商品不存在"}};
        }
        int quantity = request["quantity"];

        if (consumer->getShoppingCart().addItem(commodity, quantity))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to add item to cart"}};
        }
    }

    case static_cast<int>(CartOp::RemoveItem): // 从购物车移除商品
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity)
        {
            return {{"status", "error"}, {"message", "商品不存在"}};
        }

        if (consumer->getShoppingCart().removeItem(commodity))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to remove item from cart"}};
        }
    }

    case static_cast<int>(CartOp::UpdateQuantity): // 修改商品数量
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity)
        {
            return {{"status", "error"}, {"message", "商品不存在"}};
        }
        int newQuantity = request["quantity"];

        if (consumer->getShoppingCart().updateQuantity(commodity, newQuantity))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to update item quantity"}};
        }
    }

    case static_cast<int>(CartOp::Checkout): // 结算购物车
    {
        auto items = consumer->getShoppingCart().getItems();
        if (items.empty())
        {
            return {{"status", "error"}, {"message", "Cart is empty"}};
        }

        // 创建订单
        auto order = std::make_unique<Order>(items);
        double total = order->getTotalAmount();

        // 检查是否立即支付
        bool immediatePayment = request.value("immediatePayment", false);

        if (immediatePayment)
        {
            if (order->pay(consumer, &_userManager))
            {
                _userManager.markDirty();
                _commodityManager.markDirty();
                // unique_ptr 自动释放订单
                consumer->getShoppingCart().clear();
                return {
                    {"status", "success"},
                    {"message", "Payment successful"},
                    {"total", total}};
            }
            else
            {
                return {{"status", "error"}, {"message", "Payment failed"}};
            }
        }
        else
        {
            consumer->getOrders().push_back(std::move(order));
            consumer->getShoppingCart().clear();
            size_t orderIndex = consumer->getOrders().size() - 1;
            return {{"status", "success"}, {"orderIndex", orderIndex}, {"total", total}, {"message", "Order saved for later payment"}};
        }
    }

    default:
        return {{"status", "error"}, {"message", "Invalid cart operation"}};
    }
}

json Server::handleOrderOperation(const json &request)
{
    std::cout << "Handling order operation" << '\n';
    std::string username = request["username"];
    int operation = request["operation"];
    User *user = _userManager.getUserByUsername(username);

    if (!user || user->getUserType() != "Consumer")
    {
        return {{"status", "error"}, {"message", "Invalid consumer user"}};
    }

    Consumer *consumer = dynamic_cast<Consumer *>(user);
    if (!consumer)
    {
        return {{"status", "error"}, {"message", "User is not a consumer"}};
    }

    switch (operation)
    {
    case static_cast<int>(OrderOp::ViewPending): // 查看待支付订单
    {
        auto &orders = consumer->getOrders();
        json result = json::array();
        for (size_t i = 0; i < orders.size(); ++i)
        {
            json orderInfo = {
                {"orderIndex", i},
                {"total", orders[i]->getTotalAmount()},
                {"status", orders[i]->getStatus() == Order::PENDING ? "pending" : orders[i]->getStatus() == Order::PAID ? "paid"
                                                                                                                        : "cancelled"},
                {"items", json::array()}};

            for (const auto &item : orders[i]->getItems())
            {
                orderInfo["items"].push_back({{"name", item.first->getName()},
                                              {"quantity", item.second},
                                              {"price", item.first->getPrice()}});
            }

            result.push_back(orderInfo);
        }
        return {{"status", "success"}, {"orders", result}};
    }

    case static_cast<int>(OrderOp::Pay): // 支付订单
    {
        size_t orderIndex = request["orderIndex"];
        auto &orders = consumer->getOrders();
        if (orderIndex >= orders.size())
        {
            return {{"status", "error"}, {"message", "订单不存在"}};
        }

        if (orders[orderIndex]->pay(consumer, &_userManager))
        {
            _userManager.markDirty();
            _commodityManager.markDirty();
            orders.erase(orders.begin() + orderIndex); // unique_ptr 自动释放订单
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Payment failed"}};
        }
    }

    case static_cast<int>(OrderOp::Cancel): // 取消订单
    {
        size_t orderIndex = request["orderIndex"];
        auto &orders = consumer->getOrders();
        if (orderIndex >= orders.size())
        {
            return {{"status", "error"}, {"message", "订单不存在"}};
        }

        if (orders[orderIndex]->cancel())
        {
            _commodityManager.markDirty();
            orders.erase(orders.begin() + orderIndex); // unique_ptr 自动释放订单
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Cancel failed"}};
        }
    }

    default:
        return {{"status", "error"}, {"message", "Invalid order operation"}};
    }
}
