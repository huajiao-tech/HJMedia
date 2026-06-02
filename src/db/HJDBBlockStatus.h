//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <vector>
#include <cstdint>

// Lightweight wrapper for BLOB bytes. Kept minimal to avoid heavy includes.
struct HJDBBlockStatus {
    std::vector<uint8_t> bytes; // raw byte buffer

    size_t size() const { return bytes.size(); }
    void clear() { bytes.clear(); }
};

struct HJDBSpanInfo
{
    int block_start_idx{ 0 };       //起始块索引
    int block_cnt{ 0 };             //紧跟块数量
    std::string local_url{ "" };    //这些块数据对应的文件路径

    HJDBSpanInfo() = default;
    HJDBSpanInfo(const std::string& local_url)
        : local_url(local_url) {}
    HJDBSpanInfo(const int block_start_idx, const int block_cnt, const std::string& local_url)
        : block_start_idx(block_start_idx), block_cnt(block_cnt), local_url(local_url) {}

    bool operator==(const HJDBSpanInfo& other) const {
        return block_start_idx == other.block_start_idx &&
               block_cnt == other.block_cnt &&
               local_url == other.local_url;
    }
    bool operator!=(const HJDBSpanInfo& other) const {
        return !(*this == other);
    }
};