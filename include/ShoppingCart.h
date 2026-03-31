#ifndef SHOPPINGCART_H
#define SHOPPINGCART_H

#include <vector>

class Commodity;

// 购物车类
class ShoppingCart
{
public:
    bool addItem(Commodity *commodity, int quantity);
    bool removeItem(Commodity *commodity);
    bool updateQuantity(Commodity *commodity, int newQuantity);
    const std::vector<std::pair<Commodity *, int>> &getItems() const;
    void clear();
    double calculateTotal() const;

private:
    std::vector<std::pair<Commodity *, int>> _items;
};

#endif
