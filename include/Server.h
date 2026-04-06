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
#include "nlohmann/json.hpp"
#include "User.h"
#include "Commodity.h"

using json = nlohmann::json;

// 商家操作枚举
enum class MerchantOp
{
    ViewBalance = 1,
    Recharge = 2,
    AddCommodity = 3,
    ManageCommodities = 4,
    ChangePassword = 5,
    ListMyCommodities = 40,
    ModifyPrice = 41,
    ModifyStock = 42,
    ModifyDiscount = 43,
    BatchModifyDiscount = 44
};

// 消费者操作枚举
enum class ConsumerOp
{
    ViewBalance = 1,
    Recharge = 2,
    ViewCommodities = 3,
    SearchCommodities = 4,
    ChangePassword = 5,
    CartOperation = 6,
    OrderOperation = 7
};

// 购物车操作枚举
enum class CartOp
{
    ViewCart = 1,
    AddItem = 2,
    RemoveItem = 3,
    UpdateQuantity = 4,
    Checkout = 5
};

// 订单操作枚举
enum class OrderOp
{
    ViewPending = 0,
    Pay = 1,
    Cancel = 2
};

// 服务器类
class Server
{
public:
    // 构造函数
    Server(int port);
    // 析构函数
    ~Server();
    // 启动服务器
    void start();

private:
    int _port;                                                                    // 端口号
    std::unordered_map<std::string, std::function<json(const json &)>> _handlers; // 处理函数映射表
    UserManager _userManager;                                                     // 用户管理器
    CommodityManager _commodityManager;                                           // 商品管理器

    // 线程池
    std::vector<std::thread> _workers;        // 工作线程
    std::queue<std::function<void()>> _tasks; // 任务队列
    std::mutex _queueMutex;                   // 任务队列互斥锁
    std::condition_variable _cv;              // 条件变量
    std::atomic<bool> _stop{false};           // 停止标志
    std::shared_mutex _dataMutex;              // 读写锁（保护 _userManager/_commodityManager）

    // 处理单个连接
    void handleConnection(int socket);

    // 处理登录请求
    json handleLogin(const json &request);
    // 处理注册请求
    json handleRegister(const json &request);
    // 处理商品搜索请求
    json handleSearchCommodities(const json &request);
    // 处理用户余额操作请求
    json handleBalanceOperation(User *user, int operation, const json &request);
    // 处理密码修改请求
    json handlePasswordChange(User *user, const json &request);
    // 处理购物车操作请求
    json handleCartOperation(const json &request);
    // 处理订单操作请求
    json handleOrderOperation(const json &request);
    // 处理商家操作请求
    json handleMerchantOperation(const json &request);
    // 处理消费者操作请求
    json handleConsumerOperation(const json &request);

    // 将商品信息序列化为JSON
    static json commodityToJson(const Commodity *commodity);

    // 判断请求是否为只读操作（用于选择共享锁或独占锁）
    bool isReadOnly(const std::string &action, const json &request) const;
};

#endif
