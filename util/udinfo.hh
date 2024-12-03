#pragma once

#include <cstdint>

// 用于表示“用户数据报”信息（UDP，或TCP标头的“UDP-like”部分）的小型结构
struct UserDatagramInfo {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t cksum;
};
