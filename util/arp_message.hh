#pragma once

#include "ethernet_header.hh"
#include "ipv4_header.hh"
#include "parser.hh"

// ARP 消息（引自rfc::rfc826）
struct ARPMessage {
    static constexpr size_t LENGTH = 28;          // ARP 消息的字节数
    static constexpr uint16_t TYPE_ETHERNET = 1;  // 以太网/Wi-Fi作为链路层协议的ARP类型
    static constexpr uint16_t OPCODE_REQUEST = 1;
    static constexpr uint16_t OPCODE_REPLY = 2;

    uint16_t hardware_type = TYPE_ETHERNET;              // 链路层协议类型（通常为以太网/Wi-Fi）
    uint16_t protocol_type = EthernetHeader::TYPE_IPv4;  // 网络层协议类型（通常为IPv4）
    uint8_t hardware_address_size = sizeof(EthernetHeader::src);
    uint8_t protocol_address_size = sizeof(IPv4Header::src);
    uint16_t opcode{};  // 请求/回复类型

    EthernetAddress sender_ethernet_address{};
    uint32_t sender_ip_address{};

    EthernetAddress target_ethernet_address{};
    uint32_t target_ip_address{};

    // 返回一个字符串，包含ARP消息的可读格式
    std::string to_string() const;

    // ARP 消息类型是否被解析器支持
    bool supported() const;

    void parse(Parser& parser);
    void serialize(Serializer& serializer) const;
};
