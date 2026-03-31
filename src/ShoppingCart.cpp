#include <iostream>
#include <algorithm>
#include "ShoppingCart.h"
#include "Commodity.h"

bool ShoppingCart::addItem(Commodity *commodity, int quantity)
{
    if (quantity <= 0)
    {
        std::cerr << "数量必须大于0" << std::endl;
        return false;
    }

    auto it = std::find_if(_items.begin(), _items.end(),
                           [commodity](const auto &item)
                           { return item.first == commodity; });
    int newQuantity = (it != _items.end()) ? it->second + quantity : quantity;
    if (newQuantity > commodity->getStorage())
    {
        std::cerr << "添加失败: 商品 " << commodity->getName()
                  << " 的库存不足 (库存: " << commodity->getStorage()
                  << ", 请求数量: " << newQuantity << ")" << std::endl;
        return false;
    }

    if (it != _items.end())
    {
        it->second = newQuantity;
    }
    else
    {
        _items.emplace_back(commodity, quantity);
    }
    return true;
}

bool ShoppingCart::removeItem(Commodity *commodity)
{
    auto it = std::find_if(_items.begin(), _items.end(),
                           [commodity](const auto &item)
                           { return item.first == commodity; });

    if (it == _items.end())
    {
        std::cerr << "购物车中不存在该商品" << std::endl;
        return false;
    }

    _items.erase(it);
    return true;
}

bool ShoppingCart::updateQuantity(Commodity *commodity, int newQuantity)
{
    if (newQuantity <= 0)
    {
        std::cerr << "数量必须大于0" << std::endl;
        return false;
    }
    if (newQuantity > commodity->getStorage())
    {
        std::cerr << "更新失败: 商品 " << commodity->getName()
                  << " 的库存不足 (库存: " << commodity->getStorage()
                  << ", 修改数量: " << newQuantity << ")" << std::endl;
        return false;
    }

    auto it = std::find_if(_items.begin(), _items.end(),
                           [commodity](const auto &item)
                           { return item.first == commodity; });

    if (it == _items.end())
    {
        std::cerr << "购物车中不存在该商品" << std::endl;
        return false;
    }

    it->second = newQuantity;
    return true;
}

const std::vector<std::pair<Commodity *, int>> &ShoppingCart::getItems() const
{
    return _items;
}

void ShoppingCart::clear()
{
    _items.clear();
}

double ShoppingCart::calculateTotal() const
{
    double total = 0.0;
    for (const auto &item : _items)
    {
        total += item.first->getPrice() * item.second;
    }
    return total;
}
