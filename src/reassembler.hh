#pragma once

#include <map>

#include "byte_stream.hh"

class Reassembler {
public:
    // Construct Reassembler to write into given ByteStream.
    explicit Reassembler(ByteStream&& output) : output_(std::move(output)) {}

    /**
     * @brief 插入一个新的子串，用于组装成字节流
     * @param first_index 子串的起始索引
     * @param data 子串本身
     * @param is_last_substring 子串是否为字节流的最后一部分
     * @param output 一个可变的引用，指向写入器
     * Reassembler的工作是将索引子串（可能是无序的、重叠的）重新组装成原始字节流。
     * 当收到字节流的下一个字节时，应该将其写入输出。
     * 若收到了可以容纳在字节流的可用容量中，但不能立即写入（因为之前的字节仍未收到）的字节，应将它们暂时存储在内部，直到空隙被填补。
     * Reassembler应该丢弃超过字节流可用容量的字节（即，即使填补了空隙，也无法写入的字节），并在写入最后一个字节后关闭字节流。
     */
    void insert(uint64_t first_index, std::string data, bool is_last_substring);

    // Reassembler的缓冲区中存储的字节数
    uint64_t bytes_pending() const;

    // Reassembler的输出流的读写器，用于读取和写入字节流
    Reader& reader() { return output_.reader(); }
    const Reader& reader() const { return output_.reader(); }

    // 输出流的读写器，但只能用于读取（不能写入）
    const Writer& writer() const { return output_.writer(); }

    // 返回当前未组装的第一个索引
    inline uint64_t cur_idx() const { return cur_idx_; }

private:
    // 遍历缓冲区尝试拼接缓存的字节切片，写入到字节流
    inline void try_assemble();
    // 检查当前字节切片是否合法（是否重叠/超出字节流最大容量，若有则原地修剪为正确格式）
    // 返回值表示是否未截断字节切片的后缀（至少保留了最后一个字节），用于确定末尾切片标识
    inline bool trim_slice(std::string& data, uint64_t& idx) const;
    // 添加待拼接的字节切片到缓冲区中
    inline void add_pending_slice(std::string& data, uint64_t idx, bool is_last_substring);

    ByteStream output_;  // 待写入的目标流
    std::map<uint64_t, std::pair<std::string, bool>>
        buffer_{};  // 缓存待组装的字节（按索引排序以便于处理），pair的第二维表示是否为最后一个字节切片
    uint64_t cur_idx_{};        // 当前处理到（未写入）的最小字节流索引
    uint64_t pending_bytes_{};  // 存储在Reassembler的buffer中（未写入）的字节数
};
