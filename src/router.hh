#pragma once

#include <memory>
#include <optional>
#include <map>

#include "exception.hh"
#include "network_interface.hh"

/**
 * @brief 具有多个网络接口并在这些接口之间执行最长前缀匹配路由的路由器。
 */
class Router {
public:
    /**
     * @brief 添加一个网络接口到路由器中。
     * @param[in] interface 一个已经构造好的网络接口。
     * @returns 网络接口添加到路由器后，返回接口的索引。
     */
    size_t add_interface(std::shared_ptr<NetworkInterface> interface) {
        _interfaces.push_back(notnull("add_interface", std::move(interface)));
        return _interfaces.size() - 1;
    }

    // 通过索引访问一个网络接口
    std::shared_ptr<NetworkInterface> interface(const size_t N) { return _interfaces.at(N); }

    // 添加一个路由（转发规则）
    void add_route(uint32_t route_prefix, uint8_t prefix_length, std::optional<Address> next_hop,
                   size_t interface_num);

    // 在接口之间路由数据包
    void route();

private:
    // 路由器的网络接口集合
    std::vector<std::shared_ptr<NetworkInterface>> _interfaces{};

    // Trie树维护路由表，同时负责执行最长前缀匹配
    struct TrieNode {  // Trie树节点
        std::unique_ptr<TrieNode> children[2]{};
        bool is_end{};
        std::optional<Address> next_hop{};
        size_t interface_num{};
        TrieNode() = default;
    };
    struct RouteTrie {
        std::unique_ptr<TrieNode> root{std::make_unique<TrieNode>()};

        RouteTrie() = default;
        // 插入一条路由
        void insert(TrieNode* node, uint32_t pref, uint8_t plen, const std::optional<Address> &nhop, size_t inum, uint8_t pos);
        // 找到对于当前 IP 地址满足最长前缀匹配的路由
        TrieNode* search(TrieNode* node, uint32_t ip_addr, uint8_t pos);
    } route_trie_{};
};
