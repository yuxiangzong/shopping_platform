#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// 消息最大长度限制（防止异常数据导致过大分配）
inline constexpr uint32_t MAX_MESSAGE_SIZE = 1 << 20; // 1MB

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

// 循环发送，确保所有字节发出
inline void sendAll(int sock, const void *buf, size_t len)
{
    const char *ptr = static_cast<const char *>(buf);
    size_t remaining = len;
    while (remaining > 0)
    {
        ssize_t sent = ::send(sock, ptr, remaining, MSG_NOSIGNAL);
        if (sent <= 0)
            throw std::runtime_error("Send failed");
        ptr += sent;
        remaining -= sent;
    }
}

// 循环接收，确保读满指定长度
inline void recvAll(int sock, void *buf, size_t len)
{
    char *ptr = static_cast<char *>(buf);
    size_t remaining = len;
    while (remaining > 0)
    {
        ssize_t received = ::recv(sock, ptr, remaining, 0);
        if (received <= 0)
            throw std::runtime_error("Connection closed or recv failed");
        ptr += received;
        remaining -= received;
    }
}

// 发送 JSON：4 字节长度前缀（网络字节序）+ 消息体
inline void sendJson(int sock, const json &j)
{
    const std::string body = j.dump();
    uint32_t len = htonl(static_cast<uint32_t>(body.size()));
    sendAll(sock, &len, sizeof(len));
    sendAll(sock, body.data(), body.size());
}

// 接收 JSON：先读 4 字节长度前缀，再按长度读取消息体
inline json recvJson(int sock)
{
    uint32_t len;
    recvAll(sock, &len, sizeof(len));
    len = ntohl(len);
    if (len == 0 || len > MAX_MESSAGE_SIZE)
        throw std::runtime_error("Invalid message length: " + std::to_string(len));

    std::string body(len, '\0');
    recvAll(sock, &body[0], len);
    return json::parse(body);
}

#endif // PROTOCOL_H
