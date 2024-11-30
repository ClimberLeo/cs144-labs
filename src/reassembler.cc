#include "reassembler.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring) {
    // 剪去“无法立即写入字节流”的部分（超出最大容量/重叠），且截断后缀时取消末尾切片标识
    is_last_substring &= trim_slice(data, first_index);
    // 判断是否可直接推入流
    if (cur_idx_ == first_index) {
        output_.writer().push(data);
        cur_idx_ += data.size();
        try_assemble();           // 每次推入数据后尝试拼接缓冲区内的字节切片
        if (is_last_substring) {  // 全部写入完成，关闭流
            output_.writer().close();
        }
    } else {  // 放入缓冲区，以待后续拼接
        add_pending_slice(data, first_index, is_last_substring);
    }
}

uint64_t Reassembler::bytes_pending() const { return pending_bytes_; }

inline void Reassembler::add_pending_slice(std::string& data, uint64_t idx, bool is_last_substring) {
    // 当前切片可能会覆盖一部分的缓冲区数据，因此先删除被覆盖的重叠部分
    auto l = idx, r = idx + data.size();  // 切片覆盖的范围（左闭右开）
    auto it = buffer_.lower_bound(l);     // 找到第一个索引小于l的元素
    if (it != buffer_.begin()) {          // 检查左侧是否有重叠部分
        auto prev_it = it;
        prev_it--;
        auto& [prev_idx, prev_data] = *prev_it;
        // 存在可以完全覆盖当前切片的缓冲区切片，直接丢弃当前切片
        if (prev_idx + prev_data.first.size() >= r) {
            return;
        }
        // 切片左侧有重叠部分，截断该部分
        if (prev_idx + prev_data.first.size() > l) {
            pending_bytes_ -= prev_idx + prev_data.first.size() - l;
            prev_data.first.resize(l - prev_it->first);
            prev_data.second = false;  // 切片被截断，取消末尾切片标记
        }
    }
    // 检查是否还有完全覆盖当前切片的部分（左端点相同，但缓冲区切片大小更大）
    if (it != buffer_.end() && it->first == l && it->second.first.size() >= data.size()) {
        return;
    }
    // 此时不存在可以覆盖当前待加入切片的缓冲区切片，移除缓冲区中所有被当前切片覆盖的切片
    while (it != buffer_.end() && it->first < r) {
        auto& [b_idx, b_data] = *it;
        if (b_idx + b_data.first.size() > r) {  // 截断与后续切片重叠的部分
            data.resize(data.size() - (r - b_idx));
            is_last_substring = false;  // 切片被截断，取消末尾切片标记
            break;
        }
        pending_bytes_ -= b_data.first.size();
        it = buffer_.erase(it);
    }
    // 加入待处理切片
    pending_bytes_ += data.size();
    buffer_[idx] = {move(data), is_last_substring};
}

inline void Reassembler::try_assemble() {
    for (auto it = buffer_.begin(); it != buffer_.end();
         it = buffer_.erase(it)) {  // 遍历缓冲区，写入可被写入的切片到流
        uint64_t idx = it->first;
        auto& [data, is_last_substring] = it->second;
        // 缓冲区按索引排序，那么当前切片无法拼接则后面的切片必然也无法拼接，因此直接退出
        if (idx > cur_idx_) {
            break;
        }
        // 缓冲区存在可直接推入流的切片（注意可能是空）
        auto osz = data.size();                      // 未裁剪前的切片大小
        is_last_substring &= trim_slice(data, idx);  // 修剪切片
        output_.writer().push(data);
        pending_bytes_ -= osz;  // 由于slice可能被裁剪，此处应减去裁剪前的大小
        cur_idx_ += data.size();
        if (is_last_substring) {  // 注意这个变量是引用，不要出现内存错误
            output_.writer().close();
        }
    }
}

inline bool Reassembler::trim_slice(std::string& data, uint64_t& idx) const {
    // 丢弃重叠索引部分的字节
    if (idx < cur_idx_) {
        uint64_t overlap_size = min(data.size(), cur_idx_ - idx);
        data.erase(0, overlap_size);
        idx = cur_idx_;
    }
    // 丢弃（加上相对于下标的偏移量后）超出当前字节流最大容量部分的字节
    auto off = idx - cur_idx_;
    if (off + data.size() > output_.writer().available_capacity()) {
        data.resize(max(static_cast<int>(output_.writer().available_capacity() - off), 0));
        return false;  // 返回false表示切片后缀被截断
    }

    return true;
}
