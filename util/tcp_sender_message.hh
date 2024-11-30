#pragma once

#include <string>

#include "wrapping_integers.hh"

/**
 * @brief TCPSenderMessage
 * TCPSenderMessage结构包含从TCP发送器到接收器发送的信息。(Sender -> Receiver)
 *
 * @param seqno 段的序列号。如果设置了SYN标志，则为SYN标志的序列号。否则是负载的起始序列号。
 * @param SYN SYN(Beginning of
 * stream)标志。如果设置了，则表示字节流的开始，seqno字段包含初始序列号（ISN）--零点，注意此序列号只表示起点，不包含实际数据（实际数据从下一个序列号开始算）
 * @param payload 负载：字节流的子字符串（可能为空）。
 * @param FIN FIN(End of
 * stream)标志。如果设置了，则表示字节流的结束。同样，它只表示终点，实际数据应在前一个序列号结束
 * @param RST RST标志。如果设置了，则表示连接应当中止。
 * 注：只有ISN和绝对序列号才有SYN和FIN，且绝对序列号的SYN默认从0开始，流索引是没有的，因为其数据默认从0索引开始
 */

struct TCPSenderMessage {
    Wrap32 seqno{0};

    bool SYN{};
    std::string payload{};
    bool FIN{};

    bool RST{};

    // 这个段使用了多少个序列号？
    size_t sequence_length() const { return SYN + payload.size() + FIN; }
};
