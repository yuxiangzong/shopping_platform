#include <iostream>
#include <limits>
#include "Client.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << '\n';
        return 1;
    }

    std::string serverIp = argv[1];
    int port = std::stoi(argv[2]);
    Client client(serverIp, port);

    while (true)
    {
        client.showMainMenu();
        int choice;
        if (!(std::cin >> choice))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cerr << "无效的输入\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        try
        {
            switch (choice)
            {
            case 0:
                return 0;
            case 1:
            {
                std::cout << "请选择用户类型: 1. 商家, 2. 消费者\n";
                int userType;
                std::cin >> userType;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                if (userType != 1 && userType != 2)
                {
                    std::cout << "无效的用户类型\n";
                    break;
                }

                std::string type = (userType == 1) ? "Merchant" : "Consumer";
                std::string username, password;
                std::cout << "请输入用户名: ";
                std::getline(std::cin, username);
                std::cout << "请输入密码: ";
                std::getline(std::cin, password);

                json request = {
                    {"action", "register"},
                    {"type", type},
                    {"username", username},
                    {"password", password}};

                json response = client.sendRequest(request);
                if (response["status"] == "success")
                {
                    std::cout << username << "注册成功！\n";
                }
                else
                {
                    std::cout << "注册失败: " << response["message"] << "\n";
                }
                break;
            }
            case 2:
            {
                std::string username, password;
                std::cout << "请输入用户名: ";
                std::getline(std::cin, username);
                std::cout << "请输入密码: ";
                std::getline(std::cin, password);

                json request = {
                    {"action", "login"},
                    {"username", username},
                    {"password", password}};

                json response = client.sendRequest(request);
                if (response["status"] == "success")
                {
                    std::cout << username << "登录成功！\n";
                    std::cout << "欢迎，" << response["userType"] << "！\n";
                    if (response["userType"] == "Merchant")
                    {
                        client.showMerchantMenu(username);
                    }
                    else
                    {
                        client.showConsumerMenu(username);
                    }
                }
                else
                {
                    std::cout << "登录失败: " << response["message"] << "\n";
                }
                break;
            }
            case 3:
            {
                json request = {{"action", "searchCommodities"}};
                json response = client.sendRequest(request);
                Client::printCommodities(response);
                break;
            }
            case 4:
            {
                std::string name;
                std::cout << "请输入商品名称: ";
                std::getline(std::cin, name);

                json request = {
                    {"action", "searchCommodities"},
                    {"name", name}};

                json response = client.sendRequest(request);
                Client::printCommodities(response);
                break;
            }
            default:
                std::cout << "无效的选项\n";
                break;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "发生错误: " << e.what() << "\n";
        }
    }

    return 0;
}
