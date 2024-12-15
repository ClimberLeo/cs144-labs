#include "network_interface.hh"

#include <iostream>

#include "arp_message.hh"
#include "exception.hh"

using namespace std;

/**
 * @brief 构造函数
 * @param[in] name: 网络接口名称
 * @param[in] port: 网络接口的输出端口
 * @param[in] ethernet_address: 以太网（ARP 称为“硬件”）地址
 * @param[in] ip_address: IP（ARP 称为“协议”）地址
 */
NetworkInterface::NetworkInterface(string_view name, shared_ptr<OutputPort> port,
                                   const EthernetAddress& ethernet_address, const Address& ip_address)
    : name_(name),
      port_(notnull("OutputPort", move(port))),
      ethernet_address_(ethernet_address),
      ip_address_(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

/**
 * @brief 发送 IPv4 数据报
 * @param[in] dgram: 要发送的 IPv4 数据报
 * @param[in] next_hop: 下一跳 IP
 * 地址，通常为路由器或默认网关，但如果直接连接到与目标相同的网络，则也可能是另一台主机
 * @note 注意：Address 类型可以通过使用 Address::ipv4_numeric() 方法转换为 uint32_t（原始 32 位 IP 地址）。
 */
void NetworkInterface::send_datagram(const InternetDatagram& dgram, const Address& next_hop) {
    // Your code here.
    (void)dgram;
    (void)next_hop;
}

/**
 * @brief 接收以太网帧
 * @param[in] frame: 传入的以太网帧
 */
void NetworkInterface::recv_frame(const EthernetFrame& frame) {
    // Your code here.
    (void)frame;
}

/**
 * @brief 模拟时间流逝的辅助函数
 * @param[in] ms_since_last_tick: 上次调用此方法以来经过的毫秒数
 */
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // Your code here.
    (void)ms_since_last_tick;
}
