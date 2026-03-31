#ifndef ORDER_H
#define ORDER_H

#include <vector>
#include <utility>

class Commodity;
class User;
class UserManager;

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

    Order(const std::vector<std::pair<Commodity *, int>> &items);

    const std::vector<std::pair<Commodity *, int>> &getItems() const { return _items; }
    double getTotalAmount() const;
    bool pay(User *consumer, UserManager *userManager);
    bool cancel();
    Status getStatus() const;
    bool freezeCommodities(bool freeze);

private:
    std::vector<std::pair<Commodity *, int>> _items;
    Status _status;
};

#endif
