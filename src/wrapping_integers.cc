#include "wrapping_integers.hh"

#include <bits/stl_pair.h>

#include <algorithm>

using namespace std;

Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) {
    return Wrap32(static_cast<uint32_t>((zero_point.raw_value_ + n) & 0xffffffffu));
}

uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
    // 具体实现时checkpoint会设为reassembler中的cur_idx_（第一个未组装的索引），
    // 其作用是确定当前Wrap32表示的实际64位索引，以便于将ISN转为Stream Indices
    uint64_t n = raw_value_ - zero_point.raw_value_, l = 1, r = INT32_MAX;
    while (l + 1 < r) {  // 二分找出整数范围外最接近checkpoint的两个数
        auto mid = (l + r) / 2;
        auto v = (mid << 32) | n;  // 这里可以看出mid实际就是候选数超出整数那部分的数值
        (v < checkpoint ? l : r) = mid;
    }
    // 先确定l和r中最接近检查点的那个作为最终候选值之一，pair存值与检查点的距离和值以便比较
    pair<uint64_t, uint64_t> pl = {0, (l << 32) | n}, pr = {0, (r << 32) | n};
    pl.first = max(pl.second, checkpoint) - min(pl.second, checkpoint);
    pr.first = max(pr.second, checkpoint) - min(pr.second, checkpoint);
    auto pa = min(pl, pr);
    pair<uint64_t, uint64_t> pb = {max(n, checkpoint) - min(n, checkpoint), n};  // n是另一个最终候选值

    return min(pa, pb).second;  // 返回两个最终候选值中与检查点最接近的那个
}
