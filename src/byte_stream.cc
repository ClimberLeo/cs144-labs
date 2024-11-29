#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(uint64_t capacity) : capacity_(capacity) {}

bool Writer::is_closed() const { return closed_; }

void Writer::push(string data) {
    if (closed_) {  // 流已关闭，不能再写入数据
        return;
    }
    size_t push_len = min(data.size(), available_capacity());
    if (push_len == 0) {  // 缓冲区已无法容纳新数据或传入数据为空
        return;
    }
    if (push_len < data.size()) {
        data.resize(push_len);
    }
    buffer_.push(move(data));   // 添加字节切片为缓冲队列的新元素
    if (buffer_.size() == 1) {  // 初始化视图
        buf_view_ = buffer_.front();
    }
    total_pushed_ += push_len;  // 更新已写入的字节数
}

void Writer::close() { closed_ = true; }

uint64_t Writer::available_capacity() const { return capacity_ - reader().bytes_buffered(); }

uint64_t Writer::bytes_pushed() const { return total_pushed_; }

bool Reader::is_finished() const { return closed_ && buffer_.empty(); }

uint64_t Reader::bytes_popped() const { return total_popped_; }

string_view Reader::peek() const { return buf_view_; }

void Reader::pop(uint64_t len) {
    // 说明：pop操作结合队列和视图的特性，buf_view_恒指向缓冲区中第一个元素（字节切片）
    // 当pop_len大于整个切片大小时将切片从队列中弹出，否则只是将其从视图的前缀中移除
    //（注意此时不会修改底层数据），待到视图长度为0时再将其从队列中弹出（表示移除该元素）
    // 这种设计的点睛之笔是用字符串视图来模拟长度小于单个切片的pop操作，结合队列对整个字符串切片的移除，
    // 由于无论怎样都不会修改字节切片，最大的一部分开销得以避免，读写操作就都变成了O(1)
    size_t pop_len = min(len, bytes_buffered());
    total_popped_ += pop_len;
    while (pop_len > 0) {
        if (pop_len < buf_view_.size()) {
            // 已经小于首元素大小，此时移除视图前缀模拟对单个切片的部分pop操作
            buf_view_.remove_prefix(pop_len);
            pop_len = 0;
        } else {
            pop_len -= buf_view_.size();
            buffer_.pop();
            if (!buffer_.empty()) {
                buf_view_ = buffer_.front();
            }
        }
    }
}

uint64_t Reader::bytes_buffered() const { return total_pushed_ - total_popped_; }
