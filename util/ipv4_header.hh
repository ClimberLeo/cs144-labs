#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "parser.hh"

// IPv4 互联网数据报头（注意：不支持 IP 选项）
struct IPv4Header {
    static constexpr size_t LENGTH = 20;         // IPV4头部长度，不包括选项
    static constexpr uint8_t DEFAULT_TTL = 128;  // 默认的合理TTL值
    static constexpr uint8_t PROTO_TCP = 6;      // TCP协议号

    static constexpr uint64_t serialized_length() { return LENGTH; }

    /*
     *  IPv4 头部格式：
     *   0                   1                   2                   3
     *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |Version|  IHL  |Type of Service|          Total Length         |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |         Identification        |Flags|      Fragment Offset    |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |  Time to Live |    Protocol   |         Header Checksum       |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |                       Source Address                          |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |                    Destination Address                        |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |                    Options                    |    Padding    |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */

    // IPv4 头部字段（按照位于 IP 头部中的顺序排列）
    uint8_t ver = 4;            // IP version 
    uint8_t hlen = LENGTH / 4;  // header length (multiples of 32 bits)
    uint8_t tos = 0;            // type of service
    uint16_t len = 0;           // total length of packet
    uint16_t id = 0;            // identification number
    bool df = true;             // don't fragment flag
    bool mf = false;            // more fragments flag
    uint16_t offset = 0;        // fragment offset field
    uint8_t ttl = DEFAULT_TTL;  // time to live field
    uint8_t proto = PROTO_TCP;  // protocol field
    uint16_t cksum = 0;         // checksum field
    uint32_t src = 0;           // src address
    uint32_t dst = 0;           // dst address

    // Length of the payload
    uint16_t payload_length() const;

    // Pseudo-header's contribution to the TCP checksum
    uint32_t pseudo_checksum() const;

    // Set checksum to correct value
    void compute_checksum();

    // Return a string containing a header in human-readable format
    std::string to_string() const;

    void parse(Parser& parser);
    void serialize(Serializer& serializer) const;
};
