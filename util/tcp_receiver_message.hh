#pragma once

#include <optional>

#include "wrapping_integers.hh"

/**
 * @brief TCPReceiverMessage结构包含来自TCP接收器到其发送者的信息。(Receiver -> Sender)
 * @param ackno 确认号。TCP Receiver所需的下一个序列号。这是一个可选字段，如果TCP
 * Receiver还未收到初始序列号，它为空。
 * @param window_size 窗口大小。TCP Receiver感兴趣（接收）的序列号数量，从ackno（如果存在）开始。最大值是65,535。
 * @param RST 重置标志。如果设置说明流中出现了错误，连接应当中止。
 */

struct TCPReceiverMessage {
    std::optional<Wrap32> ackno{};
    uint16_t window_size{};
    bool RST{};
};
