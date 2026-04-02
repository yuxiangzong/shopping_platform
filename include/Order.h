#ifndef ORDER_H
#define ORDER_H

#include <unordered_map>

class User;
class UserManager;
class Commodity;

// 订单类
class Order
{
public:
    enum Status
    {
        PENDING,
        PAID,
        CANCELLED
    };

    Order(const std::unordered_map<Commodity *, int> &items);
    ~Order();

    // 获取订单中的商品列表
    const std::unordered_map<Commodity *, int> &getItems() const { return _items; }
    // 获取订单总金额
    double getTotalAmount() const;

    // 支付订单
    bool pay(User *consumer, UserManager *userManager);

    // 取消订单
    bool cancel();

    // 获取订单状态
    Status getStatus() const;

    // 冻结/解冻商品库存
    bool freezeCommodities(bool freeze);

private:
    std::unordered_map<Commodity *, int> _items;
    Status _status;
};

#endif