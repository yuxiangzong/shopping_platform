#include <iostream>
#include "User.h"
#include "Commodity.h"

int main()
{
    UserManager userManager;
    CommodityManager commodityManager;

    while (true)
    {
        std::cout << "\n***** 电商交易平台 *****\n";
        std::cout << "1. 注册\n";
        std::cout << "2. 登录\n";
        std::cout << "3. 查看商品\n";
        std::cout << "4. 搜索商品\n";
        std::cout << "0. 退出\n";
        std::cout << "请选择: ";
        int choice;
        while (!(std::cin >> choice) || choice < 0 || choice > 4)
        {
            std::cout << "无效的选项，请重新输入: ";
            std::cin.clear();
            std::cin.ignore(100, '\n');
        }
        switch (choice)
        {
        case 0:
            return 0;
        case 1:
        {
            std::cout << "请选择用户类型: 1. 商家, 2. 消费者\n";
            int userType;
            while (!(std::cin >> userType) || userType < 1 || userType > 2)
            {
                std::cout << "无效的选项，请重新输入: ";
                std::cin.clear();
                std::cin.ignore(100, '\n');
            }
            std::string type, username, password;
            if (userType == 1)
            {
                type = "Merchant";
            }
            else if (userType == 2)
            {
                type = "Consumer";
            }
            std::cout << "请输入用户名: ";
            std::getline(std::cin >> std::ws, username);
            std::cout << "请输入密码: ";
            std::getline(std::cin >> std::ws, password);
            if (userManager.registerUser(type, username, password))
            {
                std::cout << username << "注册成功！\n";
            }
            else
            {
                std::cout << "注册失败！\n";
            }
            break;
        }
        case 2:
        {
            std::string username, password;
            std::cout << "请输入用户名: ";
            std::getline(std::cin >> std::ws, username);
            std::cout << "请输入密码: ";
            std::getline(std::cin >> std::ws, password);
            User *user = userManager.login(username, password);
            if (user)
            {
                std::cout << username << "登录成功！\n";
                user->showMenu(commodityManager, userManager);
            }
            else
            {
                std::cout << "登录失败！\n";
            }
            break;
        }
        case 3:
        {
            std::vector<Commodity *> commodities = commodityManager.findCommodity("");
            commodityManager.showCommodities(commodities);
            break;
        }
        case 4:
        {
            std::string name;
            std::cout << "请输入商品名称: ";
            std::getline(std::cin >> std::ws, name);
            std::vector<Commodity *> commodities = commodityManager.findCommodity(name);
            commodityManager.showCommodities(commodities);
            break;
        }
        default:
            std::cout << "无效的选项" << std::endl;
            break;
        }
    }
}
