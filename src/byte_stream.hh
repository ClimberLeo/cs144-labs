#pragma once

#include <cstdint>
#include <queue>
#include <string>
#include <string_view>

class Reader;
class Writer;

class ByteStream {
public:
    explicit ByteStream(uint64_t capacity);

    // 用于访问ByteStream的Reader和Writer接口的辅助函数
    Reader& reader();
    const Reader& reader() const;
    Writer& writer();
    const Writer& writer() const;

    void set_error() { error_ = true; };        // 设置流错误标志
    bool has_error() const { return error_; };  // 检查流错误标志

protected:
    // 成员变量统一实现在此基类中，以供Reader和Writer接口使用
    size_t capacity_;
    bool error_{};
    bool closed_{};
    std::queue<std::string> buffer_{};  // 缓冲区，用于存储字节流数据
    std::string_view buf_view_{};       // 缓冲区首元素视图，用于提高字节流性能
    size_t total_pushed_{};             // 已写入字节数
    size_t total_popped_{};             // 已读出字节数
};

class Writer : public ByteStream {
public:
    void push(std::string data);  // 将数据推入流中，但只写入可用容量允许的字节数
    void close();                 // 标识流已结束，不再写入数据

    bool is_closed() const;               // 检查流是否已结束
    uint64_t available_capacity() const;  // 获取当前可用容量（剩余可写入字节数）
    uint64_t bytes_pushed() const;        // 对当前流写入的总字节数
};

class Reader : public ByteStream {
public:
    std::string_view peek() const;        // 获取下一个字节
    void pop(uint64_t len);               // 从流中移除len字节（FIFO）
    std::string get_bytes(uint64_t len);  // 从流中获取len字节

    bool is_finished() const;         // 检查流是否已完成（已标记关闭且所有数据已全部读出）
    uint64_t bytes_buffered() const;  // 当前缓冲区中未读出的字节数
    uint64_t bytes_popped() const;    // 当前流中累计读出的字节数
};

/*
 * read: A (provided) helper function thats peeks and pops up to `len` bytes
 * from a ByteStream Reader into a string;
 */
void read(Reader& reader, uint64_t len, std::string& out);
