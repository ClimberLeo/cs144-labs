#pragma once

#include <queue>
#include <unordered_map>

#include "address.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"

/**
 * 网络接口连接了 IP（互联网层）与以太网（网络接入层），是 TCP/IP 网络堆栈的最底层（连接 IP 与下层网络协议，如以太网）模块
 * 此模块也被重复使用，作为路由器的一部分：路由器通常有多个网络接口，其工作是在不同的接口之间路由互联网数据报
 * 网络接口将数据报文（来自“客户”，如 TCP/IP 堆栈或路由器）转换为以太网帧
 * 为了填充以太网目标地址，它查找每个数据报文的下一个 IP 跳的以太网地址，并使用`地址解析协议`(ARP)进行请求
 * 在相反的方向，网络接口接受以太网帧，检查它们是否是发给它的，若是，则根据其类型处理载荷
 * 如果是 IPv4 数据报文，则网络接口将其上升到堆栈。如果是 ARP 请求或回复，则网络接口处理帧，并根据需要学习或回复。
 */
class NetworkInterface {
public:
    // 网络接口发送以太网帧的物理输出端口的抽象
    class OutputPort {
    public:
        virtual void transmit(const NetworkInterface& sender, const EthernetFrame& frame) = 0;
        virtual ~OutputPort() = default;
    };

    // 用给定的以太网（网络接入层）和IP（互联网层）地址构造网络接口
    NetworkInterface(std::string_view name, std::shared_ptr<OutputPort> port,
        const EthernetAddress& ethernet_address, const Address& ip_address);

    // 发送一个封装在以太网帧中的互联网数据报文（如果它知道以太网目标地址）。
    // 将需要使用 ARP（地址解析协议）查找下一跳的以太网目标地址。
    // 发送是通过调用帧的`transmit()`（成员变量）完成的。
    void send_datagram(const InternetDatagram& dgram, const Address& next_hop);

    // 接收以太网帧，并相应地做出响应。
    // 如果类型是 IPv4，则将数据报文推入`datagrams_received_`队列。
    // 如果类型是 ARP 请求，则学习（缓存）来自"发送者"字段的映射，并发送 ARP 回复。
    // 如果类型是 ARP 回复，则学习来自"发送者"字段的映射。
    void recv_frame(const EthernetFrame& frame);

    // 周期性调用该函数模拟时间流逝
    void tick(size_t ms_since_last_tick);

    // 返回网络接口的名称访问器
    const std::string& name() const { return name_; }
    const OutputPort& output() const { return *port_; }
    OutputPort& output() { return *port_; }
    std::queue<InternetDatagram>& datagrams_received() { return datagrams_received_; }

private:
    // 网络接口的可读名称
    std::string name_;

    // 物理输出端口（ + 一个辅助函数`transmit`，用于对其调用来发送以太网帧）
    std::shared_ptr<OutputPort> port_;
    void transmit(const EthernetFrame& frame) const { port_->transmit(*this, frame); }

    // 网络接口的以太网地址（又称硬件、网络接入层或链路层）
    EthernetAddress ethernet_address_;

    // 网络接口的 IP 地址（又称互联网层或网络层）
    Address ip_address_;

    // 已接收的数据报文队列
    std::queue<InternetDatagram> datagrams_received_{};

    // IP 地址到以太网地址的映射缓存（用于加快 ARP 查找），同样需要记录时间戳以清除过期的条目
    std::unordered_map<uint32_t, std::pair<EthernetAddress, size_t>> arp_cache_{};

    // 缓存每个目标 IP 下还未成功发送（等待 ARP 回复）的数据报文，记录时间戳以确保五秒内不重复发送同一 IP 下的请求
    std::unordered_map<uint32_t, std::pair<std::deque<InternetDatagram>, size_t>> unsent_dgrams_{};

    // 时间戳（以毫秒为单位）
    size_t time_stamp_ms_{0};
};
