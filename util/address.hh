#pragma once

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

// IPv4地址和DNS操作的包装类 (引自man7::ip)
class Address {
public:
    // [sockaddr_storage](@ref man7::socket) 的封装类
    // 注：`sockaddr_storage` 足够存储任何类型的套接字地址（IPv4 或 IPv6）
    class Raw {
    public:
        sockaddr_storage storage{};  // 封装的结构体本身
        // NOLINTBEGIN (*-explicit-*)
        operator sockaddr*();
        operator const sockaddr*() const;
        // NOLINTEND (*-explicit-*)
    };

private:
    socklen_t _size;  // 封装地址的大小
    Raw _address{};   // 包含地址的封装 [sockaddr_storage](@ref man7::socket)

    // 从 ip/host、service/port 以及解析器的提示构造地址
    Address(const std::string& node, const std::string& service, const addrinfo& hints);

public:
    // 通过解析主机名和服务名构造地址
    Address(const std::string& hostname, const std::string& service);

    // 从点分十进制的 IP 地址字符串（如"18.243.0.1"）和数字端口构造地址
    explicit Address(const std::string& ip, std::uint16_t port = 0);

    // 从 [sockaddr *](@ref man7::socket) 构造地址
    Address(const sockaddr* addr, std::size_t size);

    // 相等性比较
    bool operator==(const Address& other) const;
    bool operator!=(const Address& other) const { return not operator==(other); }

    /* name 转换 */

    // 点分十进制的 IP 地址字符串（"18.243.0.1"）和数字端口
    std::pair<std::string, uint16_t> ip_port() const;
    // 点分十进制的 IP 地址字符串（"18.243.0.1"）
    std::string ip() const { return ip_port().first; }
    // 数字端口（主机字节序）
    uint16_t port() const { return ip_port().second; }
    // 作为整数的 IPv4 地址（即 [主机字节序](引自 man3::byteorder)）。
    uint32_t ipv4_numeric() const;
    // 从 32 位原始数字 IP 地址创建 Address。
    static Address from_ipv4_numeric(uint32_t ip_address);
    // 人类可读的字符串，例如 "8.8.8.8:53"。
    std::string to_string() const;

    
    /** name 低级操作 */

    // 底层地址存储的大小
    socklen_t size() const { return _size; }
    // 底层套接字地址存储的常量指针
    const sockaddr* raw() const { return static_cast<const sockaddr*>(_address); }
    // 安全地转换为底层 sockaddr 类型
    template <typename sockaddr_type>
    const sockaddr_type* as() const;
};

