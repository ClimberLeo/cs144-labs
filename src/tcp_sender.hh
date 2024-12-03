#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPSender {
public:
    /* 基于给定的默认重传超时时间和可能的ISN构造TCP发送器（初始将RTO设为初值） */
    TCPSender(ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms)
        : input_(std::move(input)), isn_(isn), initial_RTO_ms_(initial_RTO_ms), cur_RTO_ms_(initial_RTO_ms) {}

    /* 生成一个空的TCPSenderMessage */
    TCPSenderMessage make_empty_message() const;

    /* 从对等方的接收器接收并处理一个TCPReceiverMessage */
    void receive(const TCPReceiverMessage& msg);

    /* push和tick方法可以用来发送消息的transmit函数类型 */
    using TransmitFunction = std::function<void(const TCPSenderMessage&)>;

    /* 从出站(outbound)流推送字节 */
    void push(const TransmitFunction& transmit);

    /* 自上次调用tick()方法以来经过的毫秒数（由参数传入） */
    void tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit);

    // 访问器
    uint64_t sequence_numbers_in_flight() const;   // 未完成的序列号数量
    uint64_t consecutive_retransmissions() const;  // 当前的连续重传次数
    Writer& writer() { return input_.writer(); }
    const Writer& writer() const { return input_.writer(); }

    // 访问输入流的读取器，但只读（不能从外部读取）
    const Reader& reader() const { return input_.reader(); }

private:
    class Timer {  // 定时器，用于处理超时重传，每次发送任务时会开启定时器
    public:
        bool is_expired(uint64_t cur_ts) const { return running_ && cur_ts - start_ts_ >= timeout_ms_; }
        bool is_running() const { return running_; }
        void start(uint64_t start_ts, uint64_t timeout_ms) {
            this->running_ = true;
            this->start_ts_ = start_ts;
            this->timeout_ms_ = timeout_ms;
        }
        void stop() { this->running_ = false; }

    private:
        bool running_ = false;  // false为未启动，true为已启动
        uint64_t start_ts_{};   // 开始时间戳
        uint64_t timeout_ms_{};
    } timer_{};
    // 构造函数中初始化的成员变量
    ByteStream input_;
    Wrap32 isn_;
    uint64_t initial_RTO_ms_;
    std::queue<TCPSenderMessage> unack_msgs_{};  // 缓存所有已发送但未确认的消息
    uint64_t unack_seq_cnt_{};                   // 未确认的序列号数量
    uint64_t consecutive_cnt_{};                 // 当前的连续重传次数
    uint64_t cur_RTO_ms_;                        // 当前的重传超时时间（毫秒）
    uint64_t time_stamp_{};                      // 当前时间戳
    uint64_t win_size_ = 1;                      // 窗口大小，初始为1（最大不超过1000字节）
    bool syn_used_{};                            // 是否已发送SYN
    bool fin_used_{};                            // 是否已发送FIN
    bool empty_window_{};                        // 窗口是否为空
};
