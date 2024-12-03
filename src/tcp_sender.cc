#include "tcp_sender.hh"

#include <iostream>

#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const { return unack_seq_cnt_; }

uint64_t TCPSender::consecutive_retransmissions() const { return consecutive_cnt_; }

void TCPSender::push(const TransmitFunction& transmit) {
    // 通信已结束或无可用窗口容量，直接退出
    if (fin_used_ || win_size_ <= unack_seq_cnt_) {
        return;
    }
    // 将字节流中尽可能多的数据按单次发送的最大载荷大小分割成段并发送出去（窗口大小还足够时）
    // 计算考虑窗口大小时还要考虑所有未确认的字节，它们也算作窗口的一部分
    // 正常情况下字节流为空时不应发送消息，但考虑到第一次发送（带有SYN标识）时payload可以为空，因此用do-while循环
    TCPSenderMessage msg{};
    msg.RST = input_.has_error();
    do {
        if (!syn_used_) {  // 第一次发送，加上SYN标志
            msg.SYN = syn_used_ = true;
            msg.seqno = isn_;  // 序列号为初始序列号
        } else {               // 不是第一次发送，首个字节的序列号作为当前段的序列号
            msg.seqno = Wrap32::wrap(1 + input_.reader().bytes_popped(), isn_);
        }
        auto ava_win_sz = win_size_ - unack_seq_cnt_ - msg.SYN;  // 实际可用窗口大小
        // 此次可发送的最大载荷大小（载荷大小不算SYN和FIN，但窗口大小是算的）
        uint64_t mx_payload_size = min(ava_win_sz, static_cast<uint64_t>(TCPConfig::MAX_PAYLOAD_SIZE));
        // flag在这里用于辅助判断是否可以加FIN：若可发送载荷大小==窗口大小，则字节流剩余字节数<载荷大小时可以加FIN；
        // 如果可发送载荷大小<窗口大小，则字节流剩余字节数<=载荷大小时可以加FIN；
        bool flag = mx_payload_size == ava_win_sz;
        if (input_.writer().is_closed() && input_.reader().bytes_buffered() <= mx_payload_size - flag) {
            msg.FIN = fin_used_ = true;  // 此次发送的数据可以包含字节流所有剩余数据，加FIN标志表示结束
        }
        msg.payload.reserve(mx_payload_size);
        msg.payload = move(input_.reader().get_bytes(mx_payload_size));
        if (msg.payload.empty() && !msg.SYN && !msg.FIN) {
            return;  // 不发送不携带任何信息的数据
        }
        // 注意：由于序列号是升序生成的，因此队列中的消息保证一定是按序列号升序排列的
        unack_msgs_.push(msg);                                     // 缓存未确认消息
        unack_seq_cnt_ += msg.payload.size() + msg.SYN + msg.FIN;  // 未确认序列号
        transmit(msg);                                             // 发送
    } while (input_.reader().bytes_buffered() > 0 && win_size_ > unack_seq_cnt_ && !fin_used_);
    // 若定时器未启动，则启动一个定时器事件
    if (!timer_.is_running()) {
        timer_.start(time_stamp_, cur_RTO_ms_);
    }
}

TCPSenderMessage TCPSender::make_empty_message() const {
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap(input_.reader().bytes_popped() + syn_used_ + fin_used_, isn_);
    msg.RST = input_.has_error();
    return msg;
}

void TCPSender::receive(const TCPReceiverMessage& msg) {
    if (msg.RST) {  // 若收到RST，则设置输入流错误状态
        input_.set_error();
        return;
    }
    // lab要求：收到窗口大小为0时应将其视作1，但不能使用指数退避，因此这里用单独标识识别空窗口
    win_size_ = max(msg.window_size, static_cast<uint16_t>(1));
    empty_window_ = msg.window_size == 0;
    if (!msg.ackno.has_value()) {  // 若无ACK，则忽略
        return;
    }
    // 计算绝对ackno
    auto most_acked_abs_seqno = msg.ackno->unwrap(isn_, input_.reader().bytes_popped());
    // 忽略非法ACK（完全超出当前所有发送的序列号数）
    if (most_acked_abs_seqno > input_.reader().bytes_popped() + syn_used_ + fin_used_) {
        return;
    }
    // 若确认码完全包含当前整个消息，则将该消息从未确认消息队列中移除（转成绝对序列号后比较）
    uint64_t ack_cnt = 0;  // 记录确认的消息数
    while (!unack_msgs_.empty()) {
        const auto& m = unack_msgs_.front();  // 取出队头消息
        if (m.seqno.unwrap(isn_, input_.reader().bytes_popped()) + m.payload.size() + m.SYN + m.FIN
            > most_acked_abs_seqno) {
            break;  // 确认码不完全包含当前消息，则退出循环
        }
        unack_seq_cnt_ -= m.payload.size() + m.SYN + m.FIN;
        unack_msgs_.pop();
        ack_cnt++;
    }
    if (ack_cnt == 0) {  // 未确认任何消息，返回（不重置RTO和定时器）
        return;
    }
    cur_RTO_ms_ = initial_RTO_ms_;  // 重置RTO为初值
    consecutive_cnt_ = 0;           // 重置重传计数器
    if (!unack_msgs_.empty()) {     // 未确认消息队列不为空，重新启动定时器
        timer_.start(time_stamp_, cur_RTO_ms_);
    } else {
        timer_.stop();  // 未确认消息队列为空，停止定时器
    }
}

void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit) {
    time_stamp_ += ms_since_last_tick;
    if (!timer_.is_expired(time_stamp_)) {  // 定时器未到期
        return;
    }
    // 定时器到期，重传最早的（队头）未确认消息，并应用“指数退避”策略
    consecutive_cnt_++;
    cur_RTO_ms_ *= empty_window_ ? 1 : 2;    // 不为空窗口时，应用“指数退避”策略
    timer_.start(time_stamp_, cur_RTO_ms_);  // 重启定时器
    transmit(unack_msgs_.front());           // 重传
}
