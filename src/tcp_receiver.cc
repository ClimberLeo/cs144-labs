#include "tcp_receiver.hh"

#include <algorithm>

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message) {
    if (message.RST) {
        reader().set_error();  // 若收到RST，则设置错误状态
        return;
    }
    if (message.SYN) {
        isn_ = message.seqno;
    }
    // 序列号检查（无起始序列号或当前序列号等于起始序列号时丢弃该数据段）
    if (!isn_.has_value() || (!message.SYN && message.seqno == isn_.value())) {
        return;
    }
    bool is_last_substring = message.FIN;
    // 将payload加入到重组器中（第一个参数实际上是将绝对序列号转为了流索引），具体说明：
    // 初次设置isn时，数据要写入的流索引就是isn的绝对序列号，因此不需要-1，而其他情况下，unwrap基于isn解析出来的绝对序列号比要写入的流索引大一个数，因此需要-1
    reassembler_.insert(message.seqno.unwrap(*isn_, reassembler_.cur_idx()) - !message.SYN, message.payload,
                        is_last_substring);
}

TCPReceiverMessage TCPReceiver::send() const {
    TCPReceiverMessage msg;
    // 若重组器已完成所有数据的写入，回执信息的ackno（即下一个要发送的序列号）还要加上FIN位
    msg.ackno = isn_.has_value() ? std::optional<Wrap32>(
                    Wrap32::wrap(reassembler_.cur_idx() + 1 + reassembler_.writer().is_closed(), isn_.value()))
                                 : nullopt;
    msg.window_size = min(writer().available_capacity(), 65535ul);  // 窗口最大大小为65535
    msg.RST = writer().has_error();  // 若发生错误，则设置RST位

    return msg;
}
