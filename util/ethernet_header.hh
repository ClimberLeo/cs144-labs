#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "parser.hh"

// 以太网地址的辅助类型（一个六字节数组）
using EthernetAddress = std::array<uint8_t, 6>;

// 以太网广播地址(ff:ff:ff:ff:ff:ff)
constexpr EthernetAddress ETHERNET_BROADCAST = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

// 以太网地址的可打印表示
std::string to_string(EthernetAddress address);

// 以太网帧头
struct EthernetHeader {
    static constexpr size_t LENGTH = 14;          // 以太网帧头的长度（以字节为单位）
    static constexpr uint16_t TYPE_IPv4 = 0x800;  // IPv4类型号 (引自RFC 791)
    static constexpr uint16_t TYPE_ARP = 0x806;   // ARP类型号 (引自RFC 826)

    EthernetAddress dst;
    EthernetAddress src;
    uint16_t type;

    // 返回以太网帧头的可读字符串
    std::string to_string() const;

    void parse(Parser& parser);
    void serialize(Serializer& serializer) const;
};
