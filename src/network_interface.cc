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
 * @brief 此方法会在调用者（TCP连接或路由器）想要发送出站（outbound）IP 数据报到下一跳 IP
 * 地址时调用，网络接口在这里负责将 IP 数据包转换为以太网帧，并将其发送
 * @param[in] dgram: 要发送的 IPv4 数据报
 * @param[in] next_hop: 下一跳 IP
 * 地址，通常为路由器或默认网关，但如果直接连接到与目标相同的网络，则也可能是另一台主机
 * @note 注意：Address 类型可以通过使用 Address::ipv4_numeric() 方法转换为 uint32_t（原始 32 位 IP 地址）。
 */
void NetworkInterface::send_datagram(const InternetDatagram& dgram, const Address& next_hop) {
    if (auto it = arp_cache_.find(next_hop.ipv4_numeric()); it != arp_cache_.end()) {  // 已缓存目标 MAC 地址
        if (it->second.second + 30000 < time_stamp_ms_) {
            // 缓存时间超过30秒，移除该arp项并重新发送arp请求（这里相当于惰性删除）
            arp_cache_.erase(it);
        } else {
            // 已缓存目标 MAC 地址，直接发送数据报
            EthernetFrame eth_frame;
            eth_frame.header.src = ethernet_address_;
            eth_frame.header.dst = it->second.first;
            eth_frame.header.type = EthernetHeader::TYPE_IPv4;
            // 序列化数据报并添加到以太网帧的有效负载中
            eth_frame.payload = move(serialize<InternetDatagram>(dgram));
            transmit(eth_frame);
            return;
        }
    }
    // 查询对同一 IP 五秒内是否发送过 ARP 请求，如果是，则到期前不再发送第二次
    auto it = unsent_dgrams_.find(next_hop.ipv4_numeric());
    if (it != unsent_dgrams_.end() && it->second.second + 5000 > time_stamp_ms_) {
        return;
    }
    //发送 ARP 请求来获取目标 MAC 地址
    ARPMessage arp_request;  // 构造 ARP 请求
    arp_request.opcode = ARPMessage::OPCODE_REQUEST;
    arp_request.sender_ip_address = ip_address_.ipv4_numeric();
    arp_request.sender_ethernet_address = ethernet_address_;
    arp_request.target_ip_address = next_hop.ipv4_numeric();
    arp_request.target_ethernet_address = {};  // 目标 MAC 地址未知，设置为 0
    // 构造以太网帧，序列化 ARP 请求到有效负载中并发送
    EthernetFrame eth_frame;
    eth_frame.header.src = ethernet_address_;
    eth_frame.header.dst = ETHERNET_BROADCAST;  // 广播到所有主机
    eth_frame.header.type = EthernetHeader::TYPE_ARP;
    eth_frame.payload = move(serialize<ARPMessage>(arp_request));
    // 先缓存未发送的数据报
    if (it == unsent_dgrams_.end()) {
        unsent_dgrams_[next_hop.ipv4_numeric()] = {{dgram}, time_stamp_ms_};
    } else {
        it->second.first.push_back(dgram);  // 不要担心重复发送相同数据报，TCP会帮你处理一切
        it->second.second = time_stamp_ms_;
    }
    transmit(eth_frame);
}

/**
 * @brief 此方法在以太网帧从网络到达时被调用，用于接收以太网帧
 * @param[in] frame: 传入的以太网帧
 */
void NetworkInterface::recv_frame(const EthernetFrame& frame) {
    if (frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_) {
        return;  // 忽略目的地址不是本机的非广播帧
    }

    if (frame.header.type == EthernetHeader::TYPE_IPv4) {
        // 解析出 IPv4 数据报
        InternetDatagram dgram;
        if (!parse<InternetDatagram>(dgram, frame.payload)) {
            cerr << "ERROR: Failed to parse IPv4 datagram\n";
            return;
        }
        datagrams_received_.push(dgram);  // 放入接收队列
        return;
    }
    // 解析出 ARP 请求或响应
    ARPMessage arp_msg;
    if (!parse<ARPMessage>(arp_msg, frame.payload)) {
        cerr << "ERROR: Failed to parse ARP message\n";
        return;
    }
    // 缓存 ARP 映射记录（无论当前收到的是请求还是响应）
    arp_cache_[arp_msg.sender_ip_address] = {arp_msg.sender_ethernet_address, time_stamp_ms_};
    // 如果是 ARP 请求且请求的目的 IP 地址是本机，则返回 ARP 响应
    if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == ip_address_.ipv4_numeric()) {
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.target_ip_address = arp_msg.sender_ip_address;
        arp_reply.target_ethernet_address = arp_msg.sender_ethernet_address;
        EthernetFrame eth_frame;
        eth_frame.header.src = ethernet_address_;
        eth_frame.header.dst = arp_msg.sender_ethernet_address;
        eth_frame.header.type = EthernetHeader::TYPE_ARP;
        eth_frame.payload = move(serialize<ARPMessage>(arp_reply));
        transmit(eth_frame);
    }
    // 发送当前缓存项对应的所有未发送的 IP 数据报
    if (auto it = unsent_dgrams_.find(arp_msg.sender_ip_address); it != unsent_dgrams_.end()) {
        EthernetFrame eth_frame;
        for (const auto& dgram : it->second.first) {
            eth_frame.header.src = ethernet_address_;
            eth_frame.header.dst = arp_msg.sender_ethernet_address;
            eth_frame.header.type = EthernetHeader::TYPE_IPv4;
            eth_frame.payload = move(serialize<InternetDatagram>(dgram));
            transmit(eth_frame);
        }
        unsent_dgrams_.erase(it);  // 发送完毕，移除该条目
    }
}

/**
 * @brief 模拟时间流逝的辅助函数
 * @param[in] ms_since_last_tick: 上次调用此方法以来经过的毫秒数
 */
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    time_stamp_ms_ += ms_since_last_tick;
    //! 注：移除所有过期arp缓存项的逻辑被放到了 send_datagram()
    //! 中以惰性删除的方式实现，这样可以提高管理arp缓存项的效率（不用每次都遍历整个缓存检查）
}
