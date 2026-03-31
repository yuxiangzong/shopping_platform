#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include "Network.h"

// ==================== Server ====================

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
}

void Server::start()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    size_t addrlen = sizeof(address);
    char buffer[10240] = {0};

    // 创建socket文件描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
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
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port " << _port << std::endl;

    while (true)
    {
        // 接受新连接
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }

        // 读取请求
        int valread = recv(new_socket, buffer, 10240, 0);
        if (valread <= 0)
        {
            close(new_socket);
            continue;
        }

        try
        {
            // 解析请求
            json request = json::parse(std::string(buffer, valread));

            // 处理请求
            std::string action = request["action"];
            if (_handlers.find(action) != _handlers.end())
            {
                json response = _handlers[action](request);
                std::string responseStr = response.dump();

                // 发送响应
                send(new_socket, responseStr.c_str(), responseStr.size(), 0);
            }
            else
            {
                json errorResponse = {{"status", "error"}, {"message", "Unknown action"}};
                std::string errorResponseStr = errorResponse.dump();
                send(new_socket, errorResponseStr.c_str(), errorResponseStr.size(), 0);
            }
        }
        catch (const std::exception &e)
        {
            json errorResponse = {{"status", "error"}, {"message", e.what()}};
            std::string errorResponseStr = errorResponse.dump();
            send(new_socket, errorResponseStr.c_str(), errorResponseStr.size(), 0);
        }

        close(new_socket);
        memset(buffer, 0, sizeof(buffer));

        // 保存用户和商品数据
        if (_userManager.saveUsers() && _commodityManager.saveCommodities())
        {
            std::cout << "Users and commodities saved successfully." << std::endl;
        }
        else
        {
            std::cout << "Failed to save users or commodities." << std::endl;
        }
    }
}

json Server::handleLogin(const json &request)
{
    std::cout << "Handling login..." << std::endl;
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
    std::cout << "Handling register..." << std::endl;
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

json Server::handleBalanceOperation(User *user, int operation, const json &request) const
{
    std::cout << "Handling balance operation..." << std::endl;
    switch (operation)
    {
    case 1: // 查看余额
        return {
            {"status", "success"},
            {"balance", user->getBalance()}};

    case 2: // 充值
    {
        double amount = request["amount"];
        if (user->addBalance(amount))
        {
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

json Server::handlePasswordChange(User *user, const json &request) const
{
    std::cout << "Handling password change..." << std::endl;
    std::string newPassword = request["newPassword"];
    if (user->setPassword(newPassword))
    {
        return {{"status", "success"}};
    }
    else
    {
        return {{"status", "error"}, {"message", "Invalid password"}};
    }
}

json Server::handleSearchCommodities(const json &request)
{
    std::cout << "Handling search commodities..." << std::endl;
    std::string name = request.value("name", "");

    std::vector<Commodity *> commodities = _commodityManager.findCommodity(name);

    json result = json::array();
    for (const Commodity *commodity : commodities)
    {
        result.push_back({{"id", std::to_string(reinterpret_cast<uintptr_t>(commodity))},
                          {"name", commodity->getName()},
                          {"description", commodity->getDescription()},
                          {"type", commodity->getCommodityType()},
                          {"merchant", commodity->getMerchant()},
                          {"discount", commodity->getDiscount()},
                          {"basePrice", commodity->getBasePrice()},
                          {"price", commodity->getPrice()},
                          {"storage", commodity->getStorage()}});
    }

    return {{"status", "success"}, {"commodities", result}};
}

json Server::handleMerchantOperation(const json &request)
{
    std::cout << "Handling merchant operation..." << std::endl;
    std::string username = request["username"];
    int operation = request["operation"];
    User *user = _userManager.getUserByUsername(username);

    if (!user || user->getUserType() != "Merchant")
    {
        return {{"status", "error"}, {"message", "Invalid merchant user"}};
    }

    switch (operation)
    {
    case 1: // 查看余额
    case 2: // 充值
        return handleBalanceOperation(user, operation, request);

    case 3: // 添加商品
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

    case 40: // 获取商家商品列表
    {
        auto commodities = _commodityManager.getCommodityByMerchant(username);
        json result = json::array();
        for (const auto &commodity : commodities)
        {
            result.push_back({{"id", std::to_string(reinterpret_cast<uintptr_t>(commodity))}, // 使用字符串格式的内存地址作为临时ID
                              {"name", commodity->getName()},
                              {"type", commodity->getCommodityType()},
                              {"price", commodity->getPrice()},
                              {"storage", commodity->getStorage()},
                              {"discount", commodity->getDiscount()},
                              {"description", commodity->getDescription()}});
        }
        return {{"status", "success"}, {"commodities", result}};
    }
    case 41: // 修改价格
    {
        uintptr_t commodityId = std::stoull(std::string(request["commodityId"]));
        Commodity *commodity = reinterpret_cast<Commodity *>(commodityId);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        double newPrice = request["newPrice"];
        if (commodity->setBasePrice(newPrice))
        {
            _commodityManager.saveCommodities();
            return {
                {"status", "success"},
                {"commodity", {{"id", std::to_string(reinterpret_cast<uintptr_t>(commodity))}, {"name", commodity->getName()}, {"type", commodity->getCommodityType()}, {"price", commodity->getPrice()}, {"storage", commodity->getStorage()}, {"discount", commodity->getDiscount()}, {"description", commodity->getDescription()}}}};
        }
        else
        {
            return {{"status", "error"}, {"message", "价格修改失败"}};
        }
    }
    case 42: // 修改库存
    {
        uintptr_t commodityId = std::stoull(std::string(request["commodityId"]));
        Commodity *commodity = reinterpret_cast<Commodity *>(commodityId);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        int newStock = request["newStock"];
        if (commodity->setStorage(newStock))
        {
            _commodityManager.saveCommodities();
            return {
                {"status", "success"},
                {"commodity", {{"id", std::to_string(reinterpret_cast<uintptr_t>(commodity))}, {"name", commodity->getName()}, {"type", commodity->getCommodityType()}, {"price", commodity->getPrice()}, {"storage", commodity->getStorage()}, {"discount", commodity->getDiscount()}, {"description", commodity->getDescription()}}}};
        }
        else
        {
            return {{"status", "error"}, {"message", "库存修改失败"}};
        }
    }
    case 43: // 修改单个商品折扣
    {
        uintptr_t commodityId = std::stoull(std::string(request["commodityId"]));
        Commodity *commodity = reinterpret_cast<Commodity *>(commodityId);
        if (!commodity || commodity->getMerchant() != username)
        {
            return {{"status", "error"}, {"message", "商品不存在或无权修改"}};
        }
        double newDiscount = request["newDiscount"];
        if (commodity->setDiscount(newDiscount))
        {
            _commodityManager.saveCommodities();
            return {
                {"status", "success"},
                {"commodity", {{"id", std::to_string(reinterpret_cast<uintptr_t>(commodity))}, {"name", commodity->getName()}, {"type", commodity->getCommodityType()}, {"price", commodity->getPrice()}, {"storage", commodity->getStorage()}, {"discount", commodity->getDiscount()}, {"description", commodity->getDescription()}}}};
        }
        else
        {
            return {{"status", "error"}, {"message", "折扣修改失败"}};
        }
    }

    case 44: // 批量修改同类型商品折扣
    {
        uintptr_t commodityId = std::stoull(std::string(request["commodityId"]));
        Commodity *commodity = reinterpret_cast<Commodity *>(commodityId);
        std::string commodityType = request["commodityType"];
        double newDiscount = request["newDiscount"];
        auto commodities = _commodityManager.getCommodityByMerchant(username);
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
            _commodityManager.saveCommodities();
            return {
                {"status", "success"},
                {"message", "成功修改" + std::to_string(modifiedCount) + "个商品的折扣"},
                {"commodity", {{"id", std::to_string(reinterpret_cast<uintptr_t>(commodity))}, {"name", commodity->getName()}, {"type", commodity->getCommodityType()}, {"price", commodity->getPrice()}, {"storage", commodity->getStorage()}, {"discount", commodity->getDiscount()}, {"description", commodity->getDescription()}}}};
        }
        else
        {
            return {{"status", "error"}, {"message", "没有找到匹配类型的商品或折扣修改失败"}};
        }
    }

    case 5: // 修改密码
        return handlePasswordChange(user, request);

    default:
        return {{"status", "error"}, {"message", "Invalid operation"}};
    }
}

json Server::handleConsumerOperation(const json &request)
{
    std::cout << "Handling consumer operation" << std::endl;
    std::string username = request["username"];
    int operation = request["operation"];
    User *user = _userManager.getUserByUsername(username);

    if (!user || user->getUserType() != "Consumer")
    {
        return {{"status", "error"}, {"message", "Invalid consumer user"}};
    }

    switch (operation)
    {
    case 1: // 查看余额
    case 2: // 充值
        return handleBalanceOperation(user, operation, request);

    case 3: // 查看商品
        return handleSearchCommodities({{"action", "searchCommodities"}});

    case 4: // 搜索商品
    {
        std::string name = request.value("name", "");
        return handleSearchCommodities({{"action", "searchCommodities"}, {"name", name}});
    }

    case 5: // 修改密码
        return handlePasswordChange(user, request);

    case 6: // 购物车操作
        return handleCartOperation(request);

    case 7: // 待支付订单
        return handleOrderOperation(request);

    default:
        return {{"status", "error"}, {"message", "Invalid operation"}};
    }
}

json Server::handleCartOperation(const json &request)
{
    std::cout << "Handling cart operation" << std::endl;
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
    case 1: // 查看购物车
    {
        auto items = consumer->_shoppingCart.getItems();
        json result = json::array();
        for (const auto &item : items)
        {
            result.push_back({{"id", std::to_string(reinterpret_cast<uintptr_t>(item.first))},
                              {"name", item.first->getName()},
                              {"quantity", item.second},
                              {"price", item.first->getPrice()},
                              {"subtotal", item.first->getPrice() * item.second}});
        }
        return {{"status", "success"}, {"items", result}, {"total", consumer->_shoppingCart.calculateTotal()}};
    }

    case 2: // 添加商品到购物车
    {
        uintptr_t commodityId = std::stoull(std::string(request["commodityId"]));
        Commodity *commodity = reinterpret_cast<Commodity *>(commodityId);
        int quantity = request["quantity"];

        if (consumer->_shoppingCart.addItem(commodity, quantity))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to add item to cart"}};
        }
    }

    case 3: // 从购物车移除商品
    {
        uintptr_t commodityId = std::stoull(std::string(request["commodityId"]));
        Commodity *commodity = reinterpret_cast<Commodity *>(commodityId);

        if (consumer->_shoppingCart.removeItem(commodity))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to remove item from cart"}};
        }
    }

    case 4: // 修改商品数量
    {
        uintptr_t commodityId = std::stoull(std::string(request["commodityId"]));
        Commodity *commodity = reinterpret_cast<Commodity *>(commodityId);
        int newQuantity = request["quantity"];

        if (consumer->_shoppingCart.updateQuantity(commodity, newQuantity))
        {
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Failed to update item quantity"}};
        }
    }

    case 5: // 结算购物车
    {
        auto items = consumer->_shoppingCart.getItems();
        if (items.empty())
        {
            return {{"status", "error"}, {"message", "Cart is empty"}};
        }

        // 创建订单
        Order *order = new Order(items);
        double total = order->getTotalAmount();

        // 检查是否立即支付
        bool immediatePayment = request.value("immediatePayment", false);

        if (immediatePayment)
        {
            if (order->pay(consumer, &_userManager))
            {
                delete order;
                consumer->_shoppingCart.clear();
                return {
                    {"status", "success"},
                    {"message", "Payment successful"},
                    {"total", total}};
            }
            else
            {
                delete order;
                return {{"status", "error"}, {"message", "Payment failed"}};
            }
        }
        else
        {
            consumer->_orders.push_back(order);
            consumer->_shoppingCart.clear();
            return {{"status", "success"}, {"orderId", std::to_string(reinterpret_cast<uintptr_t>(order))}, {"total", total}, {"message", "Order saved for later payment"}};
        }
    }

    default:
        return {{"status", "error"}, {"message", "Invalid cart operation"}};
    }
}

json Server::handleOrderOperation(const json &request)
{
    std::cout << "Handling order operation" << std::endl;
    std::string username = request["username"];
    int operation = request["operation"];
    User *user = _userManager.getUserByUsername(username);

    if (!user || user->getUserType() != "Consumer")
    {
        return {{"status", "error"}, {"message", "Invalid consumer user"}};
    }

    Consumer *consumer = dynamic_cast<Consumer *>(user); // 指针转换
    if (!consumer)
    {
        return {{"status", "error"}, {"message", "User is not a consumer"}};
    }

    switch (operation)
    {
    case 0: // 查看待支付订单
    {
        auto &orders = consumer->_orders;
        json result = json::array();
        for (size_t i = 0; i < orders.size(); ++i)
        {
            json orderInfo = {
                {"orderId", std::to_string(reinterpret_cast<uintptr_t>(orders[i]))},
                {"total", orders[i]->getTotalAmount()},
                {"status", orders[i]->getStatus() == Order::PENDING ? "pending" : "completed"},
                {"items", json::array()}};

            for (const auto &item : orders[i]->getItems())
            {
                orderInfo["items"].push_back({{"commodityId", std::to_string(reinterpret_cast<uintptr_t>(item.first))},
                                              {"name", item.first->getName()},
                                              {"quantity", item.second},
                                              {"price", item.first->getPrice()}});
            }

            result.push_back(orderInfo);
        }
        return {{"status", "success"}, {"orders", result}};
    }

    case 1: // 支付订单
    {
        uintptr_t orderId = std::stoull(std::string(request["orderId"]));
        Order *order = reinterpret_cast<Order *>(orderId);

        if (order->pay(consumer, &_userManager))
        {
            // 从待支付订单列表中移除
            consumer->_orders.erase(std::remove(consumer->_orders.begin(), consumer->_orders.end(), order), consumer->_orders.end());
            delete order;
            return {{"status", "success"}};
        }
        else
        {
            return {{"status", "error"}, {"message", "Payment failed"}};
        }
    }

    case 2: // 取消订单
    {
        uintptr_t orderId = std::stoull(std::string(request["orderId"]));
        Order *order = reinterpret_cast<Order *>(orderId);

        if (order->cancel())
        {
            // 从待支付订单列表中移除
            consumer->_orders.erase(std::remove(consumer->_orders.begin(), consumer->_orders.end(), order), consumer->_orders.end());
            delete order;
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

// ==================== Client ====================

Client::Client(const std::string &serverIp, int port)
    : _serverIp(serverIp), _port(port) {}

json Client::sendRequest(const json &request) const
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[10240] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
    {
        throw std::runtime_error("Socket creation error");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(_port);

    if (inet_pton(AF_INET, _serverIp.c_str(), &serv_addr.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid address/ Address not supported");
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        throw std::runtime_error("Connection Failed");
    }

    std::string requestStr = request.dump();
    send(sock, requestStr.c_str(), requestStr.size(), 0);

    int valread = read(sock, buffer, 10240);
    close(sock);

    return json::parse(std::string(buffer, valread));
}

void Client::showMainMenu() const
{
    std::cout << "\n***** 电商交易平台客户端 *****\n";
    std::cout << "1. 注册\n";
    std::cout << "2. 登录\n";
    std::cout << "3. 查看商品\n";
    std::cout << "4. 搜索商品\n";
    std::cout << "0. 退出\n";
    std::cout << "请选择: ";
}

void Client::showMerchantMenu(const std::string &username) const
{
    while (true)
    {
        std::cout << "\n****** 商家菜单 ******\n";
        std::cout << "欢迎，商家：" << username << std::endl;
        std::cout << "1. 查看余额\n";
        std::cout << "2. 充值\n";
        std::cout << "3. 添加商品\n";
        std::cout << "4. 管理商品\n";
        std::cout << "5. 修改密码\n";
        std::cout << "0. 退出登录\n";
        std::cout << "请选择: ";

        int choice;
        while (!(std::cin >> choice) || choice < 0 || choice > 5)
        {
            std::cerr << "无效的选项，请重新输入: " << std::endl;
            std::cin.clear();
            std::cin.ignore(100, '\n');
        }
        std::cin.ignore(100, '\n');

        json request = {
            {"action", "merchantOperation"},
            {"username", username},
            {"operation", choice}};

        switch (choice)
        {
        case 0:
            return;
        case 1:
        {
            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "\n当前余额: " << response["balance"] << std::endl;
            }
            else
            {
                std::cout << "获取余额失败: " << response["message"] << std::endl;
            }
            break;
        }
        case 2:
        {
            double amount;
            std::cout << "请输入充值金额: ";
            std::cin >> amount;
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cerr << "输入不合法" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');
            request["amount"] = amount;

            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "充值成功，当前余额: " << response["balance"] << std::endl;
            }
            else
            {
                std::cout << "充值失败: " << response["message"] << std::endl;
            }
            break;
        }
        case 3:
        {
            std::cout << "请输入商品类型ID(1: Food, 2: Clothes, 3: Book, 4: Electronics): ";
            short typeID;
            std::cin >> typeID;
            if (std::cin.fail() || typeID < 1 || typeID > 4)
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cerr << "无效的商品类型" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');

            std::string type, name, description;
            switch (typeID)
            {
            case 1:
                type = "Food";
                break;
            case 2:
                type = "Clothes";
                break;
            case 3:
                type = "Book";
                break;
            case 4:
                type = "Electronics";
                break;
            }

            std::cout << "请输入商品名称: ";
            std::getline(std::cin >> std::ws, name);
            std::cout << "请输入商品描述: ";
            std::getline(std::cin >> std::ws, description);

            double price;
            std::cout << "请输入商品价格: ";
            std::cin >> price;
            if (std::cin.fail() || price <= 0)
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cout << "输入不合法，价格必须是正数" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');

            int stock;
            std::cout << "请输入商品库存: ";
            std::cin >> stock;
            if (std::cin.fail() || stock < 0)
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cout << "输入不合法" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');

            request["type"] = type;
            request["commodityName"] = name;
            request["description"] = description;
            request["price"] = price;
            request["stock"] = stock;

            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "商品：" << name << "添加成功" << std::endl;
            }
            else
            {
                std::cout << "添加商品失败: " << response["message"] << std::endl;
            }
            break;
        }
        case 4:
        {
            while (true)
            {
                json listRequest = {
                    {"action", "merchantOperation"},
                    {"username", username},
                    {"operation", 40}}; // 40表示获取商家商品列表

                json listResponse = sendRequest(listRequest);
                if (listResponse["status"] != "success")
                {
                    std::cout << "获取商品列表失败: " << listResponse["message"] << std::endl;
                    break;
                }

                json commodities = listResponse["commodities"];
                if (commodities.empty())
                {
                    std::cout << "您还没有商品，请先添加商品" << std::endl;
                    break;
                }

                std::cout << "\n===== 商品管理 =====" << std::endl;
                for (size_t i = 0; i < commodities.size(); ++i)
                {
                    std::cout << i + 1 << ". " << commodities[i]["name"]
                              << " (价格:" << commodities[i]["price"]
                              << ", 库存:" << commodities[i]["storage"] << ")" << std::endl;
                }
                std::cout << "0. 返回上级菜单" << std::endl;
                std::cout << "请选择要管理的商品: ";

                size_t choice;
                while (!(std::cin >> choice) || choice > commodities.size())
                {
                    std::cin.clear();
                    std::cin.ignore(100, '\n');
                    std::cerr << "无效的输入" << std::endl;
                    std::cout << "请选择要管理的商品: ";
                }
                std::cin.ignore(100, '\n');

                if (choice == 0)
                    break;

                json selected = commodities[choice - 1];
                std::string commodityId = selected["id"];

                while (true)
                {
                    std::cout << "\n管理商品: " << selected["name"] << std::endl;
                    std::cout << "1. 修改价格" << std::endl;
                    std::cout << "2. 修改库存" << std::endl;
                    std::cout << "3. 设置折扣" << std::endl;
                    std::cout << "0. 返回商品列表" << std::endl;
                    std::cout << "请选择操作: ";

                    int operation;
                    while (!(std::cin >> operation) || operation < 0 || operation > 3)
                    {
                        std::cin.clear();
                        std::cin.ignore(100, '\n');
                        std::cerr << "无效的输入" << std::endl;
                        std::cout << "请选择操作: ";
                    }
                    std::cin.ignore(100, '\n');

                    if (operation == 0)
                        break;

                    json modifyRequest = {
                        {"action", "merchantOperation"},
                        {"username", username},
                        {"operation", 40 + operation}, // 41-43对应修改操作
                        {"commodityId", commodityId}};

                    switch (operation)
                    {
                    case 1:
                    {
                        double newPrice;
                        std::cout << "当前价格: " << selected["price"] << std::endl;
                        std::cout << "输入新价格: ";
                        std::cin >> newPrice;
                        if (std::cin.fail())
                        {
                            std::cin.clear();
                            std::cin.ignore(100, '\n');
                            std::cerr << "无效的输入" << std::endl;
                            continue;
                        }
                        std::cin.ignore(100, '\n');
                        modifyRequest["newPrice"] = newPrice;
                        break;
                    }
                    case 2:
                    {
                        int newStock;
                        std::cout << "当前库存: " << selected["storage"] << std::endl;
                        std::cout << "输入新库存: ";
                        std::cin >> newStock;
                        if (std::cin.fail())
                        {
                            std::cin.clear();
                            std::cin.ignore(100, '\n');
                            std::cerr << "无效的输入" << std::endl;
                            continue;
                        }
                        std::cin.ignore(100, '\n');
                        modifyRequest["newStock"] = newStock;
                        break;
                    }
                    case 3:
                    {
                        std::cout << "1. 单个商品打折\n";
                        std::cout << "2. 批量打折(同类型商品)\n";
                        std::cout << "请选择折扣方式: ";

                        int discountChoice;
                        while (!(std::cin >> discountChoice) || discountChoice < 1 || discountChoice > 2)
                        {
                            std::cin.clear();
                            std::cin.ignore(100, '\n');
                            std::cerr << "无效的输入" << std::endl;
                            std::cout << "请选择折扣方式: ";
                        }
                        std::cin.ignore(100, '\n');

                        if (discountChoice == 1)
                        {
                            double newDiscount;
                            std::cout << "当前折扣: " << selected["discount"] << std::endl;
                            std::cout << "输入新折扣(0-1): ";
                            std::cin >> newDiscount;
                            if (std::cin.fail())
                            {
                                std::cin.clear();
                                std::cin.ignore(100, '\n');
                                std::cerr << "无效的输入" << std::endl;
                                continue;
                            }
                            std::cin.ignore(100, '\n');
                            modifyRequest["newDiscount"] = newDiscount;
                        }
                        else
                        {
                            double newDiscount;
                            std::cout << "当前商品类型: " << selected["type"] << std::endl;
                            std::cout << "输入新折扣(0-1): ";
                            std::cin >> newDiscount;
                            if (std::cin.fail())
                            {
                                std::cin.clear();
                                std::cin.ignore(100, '\n');
                                std::cerr << "无效的输入" << std::endl;
                                continue;
                            }
                            std::cin.ignore(100, '\n');
                            modifyRequest["newDiscount"] = newDiscount;
                            modifyRequest["commodityType"] = selected["type"];
                            modifyRequest["operation"] = 44; // 批量折扣
                        }
                        break;
                    }
                    }

                    json modifyResponse = sendRequest(modifyRequest);
                    if (modifyResponse["status"] == "success")
                    {
                        std::cout << "修改成功" << std::endl;
                        // 更新本地显示的选中商品信息
                        selected = modifyResponse["commodity"];
                    }
                    else
                    {
                        std::cout << "修改失败: " << modifyResponse["message"] << std::endl;
                    }
                }
            }
            break;
        }
        case 5:
        {
            std::string newPassword;
            std::cout << "请输入新密码: ";
            std::getline(std::cin, newPassword);
            request["newPassword"] = newPassword;

            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "密码设置成功" << std::endl;
            }
            else
            {
                std::cout << "密码设置失败: " << response["message"] << std::endl;
            }
            break;
        }
        default:
            std::cerr << "无效的选项" << std::endl;
            break;
        }
    }
}

void Client::showConsumerMenu(const std::string &username) const
{
    while (true)
    {
        std::cout << "\n****** 消费者菜单 ******\n";
        std::cout << "欢迎，用户：" << username << std::endl;
        std::cout << "1. 查看余额\n";
        std::cout << "2. 充值\n";
        std::cout << "3. 查看商品\n";
        std::cout << "4. 搜索商品\n";
        std::cout << "5. 修改密码\n";
        std::cout << "6. 购物车\n";
        std::cout << "7. 待支付订单\n";
        std::cout << "0. 退出登录\n";
        std::cout << "请选择: ";

        int choice;
        while (!(std::cin >> choice) || choice < 0 || choice > 7)
        {
            std::cerr << "无效的选项，请重新输入: " << std::endl;
            std::cin.clear();
            std::cin.ignore(100, '\n');
        }
        std::cin.ignore(100, '\n');

        json request = {
            {"action", "consumerOperation"},
            {"username", username},
            {"operation", choice}};

        switch (choice)
        {
        case 0:
            return;
        case 1:
        {
            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "\n当前余额: " << response["balance"] << std::endl;
            }
            else
            {
                std::cout << "获取余额失败: " << response["message"] << std::endl;
            }
            break;
        }
        case 2:
        {
            double amount;
            std::cout << "请输入充值金额: ";
            std::cin >> amount;
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cerr << "输入不合法" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');
            request["amount"] = amount;

            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "充值成功，当前余额: " << response["balance"] << std::endl;
            }
            else
            {
                std::cout << "充值失败: " << response["message"] << std::endl;
            }
            break;
        }
        case 3:
        {
            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "\n商品列表:\n";
                for (const auto &item : response["commodities"])
                {
                    std::cout << "名称: " << item["name"]
                              << ", 类型: " << item["type"]
                              << ", 价格: " << item["price"]
                              << ", 库存: " << item["storage"]
                              << ", 商家: " << item["merchant"] << "\n";
                }
            }
            else
            {
                std::cout << "获取商品列表失败: " << response["message"] << std::endl;
            }
            break;
        }
        case 4:
        {
            std::string name;
            std::cout << "请输入商品名称: ";
            std::getline(std::cin >> std::ws, name);
            request["name"] = name;

            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "\n搜索结果:\n";
                for (const auto &item : response["commodities"])
                {
                    std::cout << "名称: " << item["name"]
                              << ", 类型: " << item["type"]
                              << ", 价格: " << item["price"]
                              << ", 库存: " << item["storage"]
                              << ", 商家: " << item["merchant"] << "\n";
                }
            }
            else
            {
                std::cout << "搜索失败: " << response["message"] << std::endl;
            }
            break;
        }
        case 5:
        {
            std::string newPassword;
            std::cout << "请输入新密码: ";
            std::getline(std::cin, newPassword);
            request["newPassword"] = newPassword;

            json response = sendRequest(request);
            if (response["status"] == "success")
            {
                std::cout << "密码设置成功" << std::endl;
            }
            else
            {
                std::cout << "密码设置失败: " << response["message"] << std::endl;
            }
            break;
        }
        case 6:
        {
            while (true)
            {
                std::cout << "\n****** 购物车 ******\n";
                std::cout << "1. 查看购物车\n";
                std::cout << "2. 添加商品到购物车\n";
                std::cout << "3. 从购物车移除商品\n";
                std::cout << "4. 修改商品数量\n";
                std::cout << "5. 结算购物车\n";
                std::cout << "0. 返回上一级\n";
                std::cout << "请选择: ";

                int cartChoice;
                while (!(std::cin >> cartChoice) || cartChoice < 0 || cartChoice > 5)
                {
                    std::cerr << "无效的选项，请重新输入: " << std::endl;
                    std::cin.clear();
                    std::cin.ignore(100, '\n');
                }
                std::cin.ignore(100, '\n');

                json cartRequest = {
                    {"action", "cartOperation"},
                    {"username", username},
                    {"operation", cartChoice}};

                switch (cartChoice)
                {
                case 0:
                    goto end_shopping_cart;
                case 1:
                {
                    json response = sendRequest(cartRequest);
                    if (response["status"] == "success")
                    {
                        if (response["items"].empty())
                        {
                            std::cout << "购物车为空" << std::endl;
                        }
                        else
                        {
                            std::cout << "购物车商品列表:" << std::endl;
                            for (const auto &item : response["items"])
                            {
                                std::cout << item["name"] << " x" << item["quantity"]
                                          << " 单价:" << item["price"]
                                          << " 小计:" << item["price"].get<double>() * item["quantity"].get<int>() << std::endl;
                            }
                            std::cout << "总计: " << response["total"] << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "获取购物车失败: " << response["message"] << std::endl;
                    }
                    break;
                }
                case 2:
                {
                    std::string name;
                    std::cout << "请输入要添加的商品名称: ";
                    std::getline(std::cin >> std::ws, name);

                    json searchRequest = {
                        {"action", "searchCommodities"},
                        {"name", name}};
                    json searchResponse = sendRequest(searchRequest);

                    if (searchResponse["status"] != "success" || searchResponse["commodities"].empty())
                    {
                        std::cout << "未找到商品" << std::endl;
                        break;
                    }

                    auto commodities = searchResponse["commodities"];
                    if (commodities.size() > 1)
                    {
                        std::cout << "找到多个商品，请选择:" << std::endl;
                        for (size_t i = 0; i < commodities.size(); ++i)
                        {
                            std::cout << i + 1 << ". " << commodities[i]["name"]
                                      << " 价格:" << commodities[i]["price"]
                                      << " 库存:" << commodities[i]["storage"]
                                      << " 商家:" << commodities[i]["merchant"] << std::endl;
                        }
                        size_t selection;
                        std::cin >> selection;
                        std::cin.ignore(100, '\n');
                        if (selection < 1 || selection > commodities.size())
                        {
                            std::cout << "无效选择" << std::endl;
                            break;
                        }
                        cartRequest["commodityId"] = commodities[selection - 1]["id"];
                    }
                    else
                    {
                        cartRequest["commodityId"] = commodities[0]["id"];
                    }

                    int quantity;
                    std::cout << "请输入购买数量: ";
                    std::cin >> quantity;
                    std::cin.ignore(100, '\n');
                    cartRequest["quantity"] = quantity;

                    json response = sendRequest(cartRequest);
                    if (response["status"] == "success")
                    {
                        std::cout << "添加成功" << std::endl;
                    }
                    else
                    {
                        std::cout << "添加失败: " << response["message"] << std::endl;
                    }
                    break;
                }
                case 3:
                {
                    json listResponse = sendRequest({{"action", "cartOperation"},
                                                     {"username", username},
                                                     {"operation", 1}});

                    if (listResponse["status"] != "success" || listResponse["items"].empty())
                    {
                        std::cout << "购物车为空" << std::endl;
                        break;
                    }

                    std::cout << "请选择要移除的商品:" << std::endl;
                    for (size_t i = 0; i < listResponse["items"].size(); ++i)
                    {
                        std::cout << i + 1 << ". " << listResponse["items"][i]["name"]
                                  << " x" << listResponse["items"][i]["quantity"] << std::endl;
                    }
                    size_t selection;
                    std::cin >> selection;
                    std::cin.ignore(100, '\n');
                    if (selection < 1 || selection > listResponse["items"].size())
                    {
                        std::cout << "无效选择" << std::endl;
                        break;
                    }

                    cartRequest["commodityId"] = listResponse["items"][selection - 1]["id"];
                    json response = sendRequest(cartRequest);
                    if (response["status"] == "success")
                    {
                        std::cout << "移除成功" << std::endl;
                    }
                    else
                    {
                        std::cout << "移除失败: " << response["message"] << std::endl;
                    }
                    break;
                }
                case 4:
                {
                    json listResponse = sendRequest({{"action", "cartOperation"},
                                                     {"username", username},
                                                     {"operation", 1}});

                    if (listResponse["status"] != "success" || listResponse["items"].empty())
                    {
                        std::cout << "购物车为空" << std::endl;
                        break;
                    }

                    std::cout << "请选择要修改的商品:" << std::endl;
                    for (size_t i = 0; i < listResponse["items"].size(); ++i)
                    {
                        std::cout << i + 1 << ". " << listResponse["items"][i]["name"]
                                  << " 当前数量: x" << listResponse["items"][i]["quantity"] << std::endl;
                    }
                    size_t selection;
                    std::cin >> selection;
                    std::cin.ignore(100, '\n');
                    if (selection < 1 || selection > listResponse["items"].size())
                    {
                        std::cout << "无效选择" << std::endl;
                        break;
                    }

                    int newQuantity;
                    std::cout << "请输入新的数量: ";
                    std::cin >> newQuantity;
                    std::cin.ignore(100, '\n');

                    cartRequest["commodityId"] = listResponse["items"][selection - 1]["id"];
                    cartRequest["quantity"] = newQuantity;
                    json response = sendRequest(cartRequest);
                    if (response["status"] == "success")
                    {
                        std::cout << "修改成功" << std::endl;
                    }
                    else
                    {
                        std::cout << "修改失败: " << response["message"] << std::endl;
                    }
                    break;
                }
                case 5:
                {
                    json listResponse = sendRequest({{"action", "cartOperation"},
                                                     {"username", username},
                                                     {"operation", 1}});

                    if (listResponse["status"] != "success" || listResponse["items"].empty())
                    {
                        std::cout << "购物车为空" << std::endl;
                        break;
                    }

                    std::cout << "订单总金额: " << listResponse["total"] << std::endl;
                    std::cout << "确认结算? (y/n): ";
                    char confirm;
                    std::cin >> confirm;
                    std::cin.ignore(100, '\n');
                    if (confirm == 'y' || confirm == 'Y')
                    {
                        cartRequest["immediatePayment"] = true;
                        json response = sendRequest(cartRequest);
                        if (response["status"] == "success")
                        {
                            std::cout << "支付成功!" << std::endl;
                        }
                        else
                        {
                            std::cout << "支付失败: " << response["message"] << std::endl;
                        }
                    }
                    else
                    {
                        cartRequest["immediatePayment"] = false;
                        json response = sendRequest(cartRequest);
                        if (response["status"] == "success")
                        {
                            std::cout << "订单已生成，等待支付" << std::endl;
                        }
                        else
                        {
                            std::cout << "订单生成失败: " << response["message"] << std::endl;
                        }
                    }
                    break;
                }
                default:
                    std::cerr << "无效的选项" << std::endl;
                    break;
                }
            }
        end_shopping_cart:
            break;
        }
        case 7:
        {
            json response = sendRequest({{"action", "orderOperation"},
                                         {"username", username},
                                         {"operation", 0}});
            if (response["status"] != "success")
            {
                std::cerr << "获取订单失败: " << response["message"] << std::endl;
                continue;
            }
            std::cout << "\n****** 待支付订单 ******\n";
            for (size_t i = 0; i < response["orders"].size(); ++i)
            {
                std::cout << i + 1 << ". 订单总金额: " << response["orders"][i]["total"] << "\n";
            }

            std::cout << "请选择要操作的订单(0退出): ";
            size_t selection;
            std::cin >> selection;
            std::cin.ignore(100, '\n');

            if (selection == 0)
            {
                break;
            }
            else if (selection < 1 || selection > response["orders"].size())
            {
                std::cerr << "无效选择\n";
                continue;
            }

            json order = response["orders"][selection - 1];
            std::cout << "\n订单详情:\n";
            for (const auto &item : order["items"])
            {
                std::cout << item["name"] << " x" << item["quantity"]
                          << " 单价:" << item["price"]
                          << " 小计:" << item["price"].get<double>() * item["quantity"].get<int>() << std::endl;
            }
            std::cout << "总金额: " << order["total"] << "\n";
            std::cout << "1. 支付订单\n";
            std::cout << "2. 取消订单\n";
            std::cout << "请选择操作: ";

            int action;
            std::cin >> action;
            std::cin.ignore(100, '\n');

            json orderRequest = {
                {"action", "orderOperation"},
                {"username", username},
                {"orderId", order["orderId"]},
                {"operation", action}};

            switch (action)
            {
            case 1:
            {
                json payResponse = sendRequest(orderRequest);
                if (payResponse["status"] == "success")
                {
                    std::cout << "支付成功\n";
                }
                else
                {
                    std::cout << "支付失败: " << payResponse["message"] << "\n";
                }
                break;
            }
            case 2:
            {
                json cancelResponse = sendRequest(orderRequest);
                if (cancelResponse["status"] == "success")
                {
                    std::cout << "订单已取消\n";
                }
                else
                {
                    std::cout << "取消失败: " << cancelResponse["message"] << "\n";
                }
                break;
            }
            default:
                std::cerr << "无效操作\n";
                break;
            }
        }
        break;
        default:
            std::cerr << "无效的选项" << std::endl;
            break;
        }
    }
}
