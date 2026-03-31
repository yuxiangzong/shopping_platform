#ifndef SHOPPINGCART_H
#define SHOPPINGCART_H

#include <vector>

class Commodity;

// 购物车类
class ShoppingCart
{
public:
    // 添加商品到购物车
    bool addItem(Commodity *commodity, int quantity);

    // 从购物车移除商品
    bool removeItem(Commodity *commodity);

    // 修改购物车中商品数量
    bool updateQuantity(Commodity *commodity, int newQuantity);

    // 获取购物车中所有商品
    const std::vector<std::pair<Commodity *, int>> &getItems() const;

    // 清空购物车
    void clear();

    // 计算购物车总金额
    double calculateTotal() const;

private:
    std::vector<std::pair<Commodity *, int>> _items; // 存储商品及其数量
};

#endif
