#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <vector>

// 订单项
struct OrderItem
{
    std::string commodityName;
    int quantity;
    double unitPrice;
};

// 订单类（纯数据持有者）
class Order
{
public:
    enum Status
    {
        PENDING,
        PAID,
        CANCELLED
    };

    Order(int id, int userId, Status status, const std::string &createdAt,
          std::vector<OrderItem> items)
        : _id(id), _userId(userId), _status(status),
          _createdAt(createdAt), _items(std::move(items)) {}

    int getId() const { return _id; }
    int getUserId() const { return _userId; }
    Status getStatus() const { return _status; }
    const std::string &getCreatedAt() const { return _createdAt; }
    const std::vector<OrderItem> &getItems() const { return _items; }

    double getTotalAmount() const
    {
        double total = 0.0;
        for (const auto &item : _items)
            total += item.unitPrice * item.quantity;
        return total;
    }

private:
    int _id;
    int _userId;
    Status _status;
    std::string _createdAt;
    std::vector<OrderItem> _items;
};

#endif
