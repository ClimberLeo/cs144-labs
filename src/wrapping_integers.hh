#pragma once

#include <cstdint>

/**
 * @brief Wrap32类表示一个32位无符号整数，它：
 *    - 从一个任意的“零点”（初始值）开始，
 *    - 当它达到2^32 - 1时，会回到零。
 * 这个类的目的是将TCP头部中原本的64位Stream Indices转为32位的ISN，以增加网络传输速度
 *（TCP头部中的空间是宝贵的，要尽量减少传输数据量）
 */

class Wrap32 {
public:
    explicit Wrap32(uint32_t raw_value) : raw_value_(raw_value) {}

    /* 通过给定的绝对序列号n和零点，构造一个Wrap32。 */
    static Wrap32 wrap(uint64_t n, Wrap32 zero_point);

    /**
     * unwrap方法返回一个绝对序列号，该序列号包装了Wrap32，给定零点和一个“检查点”：另一个接近所需答案的绝对序列号。
     * 有许多可能的绝对序列号包装同一个Wrap32。unwrap方法应返回与检查点最接近的那个。
     */
    uint64_t unwrap(Wrap32 zero_point, uint64_t checkpoint) const;

    Wrap32 operator+(uint32_t n) const { return Wrap32{raw_value_ + n}; }
    bool operator==(const Wrap32& other) const { return raw_value_ == other.raw_value_; }

protected:
    uint32_t raw_value_{};
};
