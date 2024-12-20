#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

/**
 * @brief 添加一条路由规则，供以后使用
 * @param route_prefix 用于匹配数据报目标地址的 IPv4 前缀，最多32位
 * @param prefix_length 路由前缀的长度，即需要匹配的目标地址的前几个高位
 * @param next_hop 下一跳地址，即数据报应该转发到的地址，如果为空，则说明网络直接连接到路由器，此时下一跳地址应该是数据报的最终目的地
 * @param interface_num 发送数据报所使用的接口索引
 */
void Router::add_route(const uint32_t route_prefix, const uint8_t prefix_length, const optional<Address> next_hop,
    const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/"
        << static_cast<int>(prefix_length) << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)")
        << " on interface " << interface_num << "\n";

    uint32_t rpref = route_prefix & static_cast<uint32_t>(~((1ll << (32 - prefix_length)) - 1));  // 裁去不在匹配范围的部分
    // 从根节点开始向下插入
    route_trie_.insert(route_trie_.root.get(), rpref, prefix_length, next_hop, interface_num, 32);
}

// 遍历所有接口，并将每个传入的数据报路由到其适当的出口接口。
void Router::route() {
    for (const auto& interface : _interfaces) {  // 遍历所有接口
        auto &q = interface->datagrams_received();  // 获取该接口的接收队列
        while (!q.empty()) {  // 转发该接口接收到的所有数据报到对应的outbound接口
            auto &dgram = q.front();
            // 执行最长前缀匹配算法，找到匹配的路由规则
            // 从根节点（位置为32，表示前缀长度为零的特殊路由规则）开始向下查找
            auto* route = route_trie_.search(route_trie_.root.get(), dgram.header.dst, 32);
            if (route != nullptr) {  // 找到匹配的路由规则
                const auto& [a, b, next_hop, inum] = *route;
                const auto& out_interface = _interfaces.at(inum);
                // TTL 到期则丢弃数据报
                if (dgram.header.ttl <= 1) {
                    break;
                }
                dgram.header.ttl--;  // 减少 TTL 值
                dgram.header.compute_checksum();  // 由于改动了头部中的信息，因此需要重新计算校验和
                if (next_hop.has_value()) {  // 路由规则中有下一跳（路由器）地址
                    out_interface->send_datagram(dgram, *next_hop);
                } else {  // 无下一跳地址时说明网络直连到路由器，直接转发到目的地址
                    out_interface->send_datagram(dgram, Address::from_ipv4_numeric(dgram.header.dst));
                }
            }
            q.pop();
        }
    }
}

/*----------------------------- Trie 树相关实现 --------------------------*/

/**
 * @brief 插入一条路由规则到 Trie 树中，从根节点开始向下查找，遇到空结点时直接创建新节点，直到到达 plen 所限定的位置为止
 * @param node 要插入的节点
 * @param perf 路由前缀的前缀部分
 * @param plen 路由前缀的长度
 * @param nhop 下一跳地址（作为叶子结点的有效元素）
 * @param inum 发送数据报所使用的接口索引 (作为叶子结点的有效元素)
 * @param pos 下一个子节点对应的位置
 */
void Router::RouteTrie::insert(TrieNode* node, uint32_t pref, uint8_t plen, const std::optional<Address> &nhop, size_t inum, uint8_t pos) {
    if (32 - pos == plen) {  // 到达叶节点
        node->is_end = true;
        node->next_hop = nhop;
        node->interface_num = inum;
        return;
    }

    pos -= 1;  // 指向下一个位置
    int b = (pref >> pos) & 1;  // 下一个位置的比特位
    if (node->children[b] == nullptr) {
        node->children[b] = make_unique<TrieNode>();
    }
    // 递归插入下一层结点
    insert(node->children[b].get(), pref, plen, nhop, inum, pos);
}

/**
 * @brief 从 Trie 的根结点开始向下执行最长前缀匹配查找（顺着 ip_addr 的每一位向下查找，找到的最后一个 is_end 结点就是最长前缀匹配的路由规则）
 * @param node 要查找的节点
 * @param ip_addr 要匹配的目标地址
 * @param pos 当前位置
 * @return 匹配的路由规则的叶子结点，或 nullptr（表示未匹配到合法路由规则）
 */
Router::TrieNode* Router::RouteTrie::search(TrieNode* node, uint32_t ip_addr, uint8_t pos) {
    if (node == nullptr) {
        return nullptr;
    }

    pos -= 1;
    bool b = (ip_addr >> pos) & 1;
    auto* ret = search(node->children[b].get(), ip_addr, pos);
    if (ret == nullptr && !node->is_end) {
        return nullptr;
    }
    return ret ? ret : node;
}
