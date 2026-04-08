#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <csignal>
#include <cstring>
#include <iostream>
#include "Server.h"

json Server::commodityToJson(const Commodity *commodity)
{
    return {{"name", commodity->getName()},
            {"type", commodity->getType()},
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

    return false;
}

Server::Server(int port)
    : _port(port),
      _db("./shopping.db"),
      _userManager(_db.handle()),
      _commodityManager(_db.handle()),
      _orderManager(_db.handle(), _userManager, _commodityManager)
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

    // 启动订单超时检查线程
    _expiryThread = std::thread(&Server::expiryCheckLoop, this);
}

Server::~Server()
{
    _stop = true;
    _cv.notify_all();
    _expiryCv.notify_all();
    for (auto &worker : _workers)
    {
        if (worker.joinable())
            worker.join();
    }
    if (_expiryThread.joinable())
        _expiryThread.join();
}

void Server::expiryCheckLoop()
{
    std::cout << "Order expiry checker started (30 min timeout, checking every 60s)." << '\n';
    while (!_stop)
    {
        std::unique_lock<std::mutex> lock(_expiryMutex);
        if (_expiryCv.wait_for(lock, std::chrono::seconds(60), [this] { return _stop.load(); }))
            break;

        // 加写锁执行取消操作
        std::unique_lock<std::shared_mutex> dataLock(_dataMutex);
        int count = _orderManager.cancelExpiredOrders(30);
        if (count > 0)
            std::cout << "Auto-cancelled " << count << " expired order(s)." << '\n';
    }
    std::cout << "Order expiry checker stopped." << '\n';
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
            std::shared_lock<std::shared_mutex> lock(_dataMutex);
            auto it = _handlers.find(action);
            response = (it != _handlers.end())
                           ? it->second(request)
                           : json({{"status", "error"}, {"message", "Unknown action"}});
        }
        else
        {
            std::unique_lock<std::shared_mutex> lock(_dataMutex);
            auto it = _handlers.find(action);
            response = (it != _handlers.end())
                           ? it->second(request)
                           : json({{"status", "error"}, {"message", "Unknown action"}});
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
        }
    }

    close(clientSocket);
}

void Server::start()
{
    int server_fd, new_socket;

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

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    signal(SIGPIPE, SIG_IGN);

    if (listen(server_fd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port " << _port << '\n';

    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }

        struct timeval timeout{.tv_sec = 30, .tv_usec = 0};
        setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

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
        return {{"status", "success"}, {"userType", user->getUserType()}};
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

    if (_userManager.registerUser(type, username, password))
    {
        return {{"status", "success"}};
    }
    else
    {
        return {{"status", "error"}, {"message", "User already exists or invalid input"}};
    }
}

json Server::handleBalanceOperation(User *user, int operation, const json &request)
{
    std::cout << "Handling balance operation..." << '\n';
    switch (operation)
    {
    case static_cast<int>(MerchantOp::ViewBalance):
        return {{"status", "success"}, {"balance", user->getBalance()}};

    case static_cast<int>(MerchantOp::Recharge):
    {
        double amount = request["amount"];
        if (user->addBalance(amount))
        {
            _userManager.updateBalance(user);
            return {{"status", "success"}, {"balance", user->getBalance()}};
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
        _userManager.updatePassword(user);
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
    case static_cast<int>(MerchantOp::ViewBalance):
    case static_cast<int>(MerchantOp::Recharge):
        return handleBalanceOperation(user, operation, request);

    case static_cast<int>(MerchantOp::AddCommodity):
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

    case static_cast<int>(MerchantOp::ListMyCommodities):
    {
        auto commodities = _commodityManager.getCommodityByMerchant(username);
        json result = json::array();
        for (const auto &commodity : commodities)
        {
            result.push_back(commodityToJson(commodity));
        }
        return {{"status", "success"}, {"commodities", result}};
    }

    case static_cast<int>(MerchantOp::ModifyPrice):
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        double newPrice = request["newPrice"];
        if (_commodityManager.updatePrice(commodity, newPrice))
        {
            return {{"status", "success"}, {"commodity", commodityToJson(commodity)}};
        }
        else
        {
            return {{"status", "error"}, {"message", "价格修改失败"}};
        }
    }

    case static_cast<int>(MerchantOp::ModifyStock):
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        int newStock = request["newStock"];
        if (_commodityManager.updateStock(commodity, newStock))
        {
            return {{"status", "success"}, {"commodity", commodityToJson(commodity)}};
        }
        else
        {
            return {{"status", "error"}, {"message", "库存修改失败"}};
        }
    }

    case static_cast<int>(MerchantOp::ModifyDiscount):
    {
        std::string commodityName = request["commodityName"];
        Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        double newDiscount = request["newDiscount"];
        if (_commodityManager.updateDiscount(commodity, newDiscount))
        {
            return {{"status", "success"}, {"commodity", commodityToJson(commodity)}};
        }
        else
        {
            return {{"status", "error"}, {"message", "折扣修改失败"}};
        }
    }

    case static_cast<int>(MerchantOp::BatchModifyDiscount):
    {
        std::string commodityType = request["commodityType"];
        double newDiscount = request["newDiscount"];
        int count = _commodityManager.batchUpdateDiscount(username, commodityType, newDiscount);
        if (count > 0)
        {
            return {{"status", "success"}, {"message", "成功修改" + std::to_string(count) + "个商品的折扣"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "没有找到匹配类型的商品或折扣修改失败"}};
        }
    }

    case static_cast<int>(MerchantOp::ChangePassword):
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
    case static_cast<int>(ConsumerOp::ViewBalance):
    case static_cast<int>(ConsumerOp::Recharge):
        return handleBalanceOperation(user, operation, request);

    case static_cast<int>(ConsumerOp::ViewCommodities):
        return handleSearchCommodities({{"action", "searchCommodities"}});

    case static_cast<int>(ConsumerOp::SearchCommodities):
    {
        std::string name = request.value("name", "");
        return handleSearchCommodities({{"action", "searchCommodities"}, {"name", name}});
    }

    case static_cast<int>(ConsumerOp::ChangePassword):
        return handlePasswordChange(user, request);

    case static_cast<int>(ConsumerOp::CartOperation):
        return handleCartOperation(request);

    case static_cast<int>(ConsumerOp::OrderOperation):
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

    int userId = user->getId();

    switch (operation)
    {
    case static_cast<int>(CartOp::ViewCart):
    {
        auto items = _orderManager.getCartItems(userId);
        json result = json::array();
        double total = 0.0;
        for (const auto &item : items)
        {
            double subtotal = item.unitPrice * item.quantity;
            result.push_back({{"name", item.commodityName},
                              {"quantity", item.quantity},
                              {"price", item.unitPrice},
                              {"subtotal", subtotal}});
            total += subtotal;
        }
        return {{"status", "success"}, {"items", result}, {"total", total}};
    }

    case static_cast<int>(CartOp::AddItem):
    {
        std::string commodityName = request["commodityName"];
        int quantity = request["quantity"];
        if (_orderManager.addToCart(userId, commodityName, quantity))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to add item to cart"}};
        }
    }

    case static_cast<int>(CartOp::RemoveItem):
    {
        std::string commodityName = request["commodityName"];
        if (_orderManager.removeFromCart(userId, commodityName))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to remove item from cart"}};
        }
    }

    case static_cast<int>(CartOp::UpdateQuantity):
    {
        std::string commodityName = request["commodityName"];
        int newQuantity = request["quantity"];
        if (_orderManager.updateCartQuantity(userId, commodityName, newQuantity))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to update item quantity"}};
        }
    }

    case static_cast<int>(CartOp::Checkout):
    {
        bool immediatePayment = request.value("immediatePayment", false);
        std::string errorMsg;
        int orderId = _orderManager.checkout(userId, immediatePayment, errorMsg);
        if (orderId < 0)
        {
            return {{"status", "error"}, {"message", errorMsg}};
        }

        if (immediatePayment)
        {
            return {{"status", "success"}, {"message", "Payment successful"}, {"orderId", orderId}};
        }
        else
        {
            return {{"status", "success"}, {"orderId", orderId}, {"message", "Order saved for later payment"}};
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

    int userId = user->getId();

    switch (operation)
    {
    case static_cast<int>(OrderOp::ViewPending):
    {
        auto orders = _orderManager.getOrders(userId);
        json result = json::array();
        for (const auto &order : orders)
        {
            std::string statusStr = (order.getStatus() == Order::PENDING) ? "pending"
                                    : (order.getStatus() == Order::PAID)  ? "paid"
                                                                          : "cancelled";
            json orderInfo = {
                {"orderId", order.getId()},
                {"total", order.getTotalAmount()},
                {"status", statusStr},
                {"createdAt", order.getCreatedAt()},
                {"items", json::array()}};

            for (const auto &item : order.getItems())
            {
                orderInfo["items"].push_back({{"name", item.commodityName},
                                              {"quantity", item.quantity},
                                              {"price", item.unitPrice}});
            }
            result.push_back(orderInfo);
        }
        return {{"status", "success"}, {"orders", result}};
    }

    case static_cast<int>(OrderOp::Pay):
    {
        int orderId = request["orderId"];
        std::string errorMsg;
        if (_orderManager.payOrder(userId, orderId, errorMsg))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", errorMsg}};
        }
    }

    case static_cast<int>(OrderOp::Cancel):
    {
        int orderId = request["orderId"];
        std::string errorMsg;
        if (_orderManager.cancelOrder(userId, orderId, errorMsg))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", errorMsg}};
        }
    }

    default:
        return {{"status", "error"}, {"message", "Invalid order operation"}};
    }
}
