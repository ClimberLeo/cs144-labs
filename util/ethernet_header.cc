#include "ethernet_header.hh"

#include <iomanip>
#include <sstream>

using namespace std;

// 返回以十六进制表示的以太网地址的文本表示
string to_string(const EthernetAddress address) {
    stringstream ss{};
    for (size_t index = 0; index < address.size(); index++) {
        ss.width(2);
        ss << setfill('0') << hex << static_cast<int>(address.at(index));
        if (index != address.size() - 1) {
            ss << ":";
        }
    }
    return ss.str();
}

// 返回以太网帧头的文本表示
string EthernetHeader::to_string() const {
    stringstream ss{};
    ss << "dst=" << ::to_string(dst);
    ss << " src=" << ::to_string(src);
    ss << " type=";
    switch (type) {
        case TYPE_IPv4:
            ss << "IPv4";
            break;
        case TYPE_ARP:
            ss << "ARP";
            break;
        default:
            ss << "[unknown type " << hex << type << "!]";
            break;
    }

    return ss.str();
}

void EthernetHeader::parse(Parser& parser) {
    // 读取目的地址
    for (auto& b : dst) {
        parser.integer(b);
    }

    // 读取源地址
    for (auto& b : src) {
        parser.integer(b);
    }

    // 读取帧类型（例如IPv4、ARP或其他）
    parser.integer(type);
}

void EthernetHeader::serialize(Serializer& serializer) const {
    // 写入目的地址
    for (const auto& b : dst) {
        serializer.integer(b);
    }

    // 写入源地址
    for (const auto& b : src) {
        serializer.integer(b);
    }

    // 写入帧类型
    serializer.integer(type);
}
