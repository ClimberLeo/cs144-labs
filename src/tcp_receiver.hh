#pragma once

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPReceiver {
public:
    // 基于给定的Reassembler构造TCPReceiver
    explicit TCPReceiver(Reassembler&& reassembler) : reassembler_(std::move(reassembler)) {}

    // 收到TCPSenderMessages，将其负载以正确的流索引插入Reassembler中。
    void receive(TCPSenderMessage message);

    // 发送TCPReceiverMessages给对端的TCPSender。
    TCPReceiverMessage send() const;

    // 访问输出（只有Reader是可访问的非const）。
    const Reassembler& reassembler() const { return reassembler_; }
    Reader& reader() { return reassembler_.reader(); }
    const Reader& reader() const { return reassembler_.reader(); }
    const Writer& writer() const { return reassembler_.writer(); }

private:
    Reassembler reassembler_;
    std::optional<Wrap32> isn_{};  // 数据的起始“零点”
};
