#include <iostream>
#include <unordered_map>
#include "Order.h"
#include "Commodity.h"
#include "User.h"

Order::Order(const std::unordered_map<Commodity *, int> &items)
    : _items(items), _status(PENDING)
{
    // 创建订单时冻结商品库存
    freezeCommodities(true);
}

Order::~Order()
{
    // 如果订单仍为待支付状态，解冻商品库存
    if (_status == PENDING)
    {
        freezeCommodities(false);
    }
}

double Order::getTotalAmount() const
{
    double total = 0.0;
    for (const auto &item : _items)
    {
        total += item.first->getPrice() * item.second;
    }
    return total;
}

bool Order::pay(User *consumer, UserManager *userManager)
{
    if (_status != PENDING)
    {
        std::cerr << "订单状态错误，无法支付" << '\n';
        return false;
    }

    // 按商家分组计算金额
    std::unordered_map<std::string, double> merchantAmounts;
    for (const auto &item : _items)
    {
        Commodity *commodity = item.first;
        merchantAmounts[commodity->getMerchant()] += commodity->getPrice() * item.second;
    }

    // 先扣消费者总金额
    double total = getTotalAmount();
    if (!consumer->subtractBalance(total))
    {
        std::cerr << "支付失败，当前余额为：" << consumer->getBalance() << '\n';
        return false;
    }

    // 为商品对应商家转账
    for (const auto &[merchantName, amount] : merchantAmounts)
    {
        User *merchant = userManager->getUserByUsername(merchantName);
        if (merchant && merchant->addBalance(amount))
        {
            std::cout << "向商家" << merchantName << "转账成功，金额为：" << amount << '\n';
        }
    }

    _status = PAID;
    return true;
}

bool Order::cancel()
{
    if (_status != PENDING)
    {
        std::cerr << "订单状态错误，无法取消" << '\n';
        return false;
    }

    _status = CANCELLED;
    // 取消订单后解冻商品库存
    freezeCommodities(false);
    return true;
}

Order::Status Order::getStatus() const
{
    return _status;
}

bool Order::freezeCommodities(bool freeze)
{
    for (auto &item : _items)
    {
        Commodity *commodity = item.first;
        int quantity = item.second;
        int newStorage = freeze ? commodity->getStorage() - quantity : commodity->getStorage() + quantity;

        if (newStorage < 0)
        {
            std::cerr << "商品库存不足，无法冻结" << '\n';
            return false;
        }

        if (!commodity->setStorage(newStorage))
        {
            std::cerr << "商品库存设置失败" << '\n';
            return false;
        }
    }
    return true;
}