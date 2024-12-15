#pragma once

#include <queue>

#include "address.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"

// This module is the lowest layer of a TCP/IP stack
// (connecting IP with the lower-layer network protocol,
// e.g. Ethernet). But the same module is also used repeatedly
// as part of a router: a router generally has many network
// interfaces, and the router's job is to route Internet datagrams
// between the different interfaces.

// The network interface translates datagrams (coming from the
// "customer," e.g. a TCP/IP stack or router) into Ethernet
// frames. To fill in the Ethernet destination address, it looks up
// the Ethernet address of the next IP hop of each datagram, making
// requests with the [Address Resolution Protocol](\ref rfc::rfc826).
// In the opposite direction, the network interface accepts Ethernet
// frames, checks if they are intended for it, and if so, processes
// the the payload depending on its type. If it's an IPv4 datagram,
// the network interface passes it up the stack. If it's an ARP
// request or reply, the network interface processes the frame
// and learns or replies as necessary.
/**
 * 网络接口连接了IP（互联网层）与以太网（网络接入层），是TCP/IP堆栈的最底层（连接IP与下层网络协议，如以太网）模块
 * 此模块也被重复使用，作为路由器的一部分：路由器通常有多个网络接口，其工作是在不同的接口之间路由互联网数据报
 * 网络接口将数据报文（来自“客户”，如TCP/IP堆栈或路由器）转换为以太网帧
 * 为了填充以太网目标地址，它查找每个数据报文的下一个IP跳的以太网地址，并使用`地址解析协议`(ARP)进行请求
 * 在相反的方向，网络接口接受以太网帧，检查它们是否是发给它的，若是，则根据其类型处理载荷
 * 如果是IPv4数据报文，则网络接口将其上升到堆栈。如果是ARP请求或回复，则网络接口处理帧，并根据需要学习或回复。
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
    // 将需要使用ARP（地址解析协议）查找下一跳的以太网目标地址。
    // 发送是通过调用帧的`transmit()`（成员变量）完成的。
    void send_datagram(const InternetDatagram& dgram, const Address& next_hop);

    // 接收以太网帧，并相应地做出响应。
    // 如果类型是IPv4，则将数据报文推入`datagrams_received_`队列。
    // 如果类型是ARP请求，则学习（缓存）来自"发送者"字段的映射，并发送ARP回复。
    // 如果类型是ARP回复，则学习来自"发送者"字段的映射。
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

    // 物理输出端口（+一个辅助函数`transmit`，用于对其调用来发送以太网帧）
    std::shared_ptr<OutputPort> port_;
    void transmit(const EthernetFrame& frame) const { port_->transmit(*this, frame); }

    // 网络接口的以太网地址（又称硬件、网络接入层或链路层）
    EthernetAddress ethernet_address_;

    // 网络接口的IP地址（又称互联网层或网络层）
    Address ip_address_;

    // 已接收的数据报文队列
    std::queue<InternetDatagram> datagrams_received_{};
};
