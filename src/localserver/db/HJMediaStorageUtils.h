//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJBlob.h"
#include "HJServerUtils.h"
#include <algorithm>

NS_HJ_BEGIN
//***********************************************************************************//
#define HJ_DB_TABLE_MAIN_NAME "category"
#define HJ_DB_TABLE_GIFTS_NAME "gifts"
#define HJ_DB_TABLE_MEDIAS_NAME "medias"
//***********************************************************************************//
// 类别信息结构体
struct HJDBCategoryInfo
{
    std::string name{""};       // 主键, 类别名称
    int type{0};                // 类型
    int64_t max_size{0};        // 类别限制大小
    int file_count{0};          // 类别文件数量
    int64_t total_size{0};      // 类别总存储占用空间大小

    bool operator==(const HJDBCategoryInfo& other) const {
        return name == other.name &&
               type == other.type &&
               max_size == other.max_size &&
               file_count == other.file_count &&
               total_size == other.total_size;
    }
};

// 文件信息结构体
struct HJDBFileInfo
{
    std::string rid{""};        // 主键, 文件唯一标识
    std::string url{""};        // 文件网络URL
    std::string local_url{""};  // 文件本地URL
    int status{0};              // 文件状态 0:未知, 1:进行中, 2:已完成
    int64_t size{0};            // 已完成字节数（用于进度）
    int64_t total_length{0};    // 总字节数
    std::string category{""};   // 文件所属类别
    int64_t create_time{0};     // 创建时间
    int64_t modify_time{0};     // 修改时间
    int use_count{0};           // 使用次数
    int level{0};               // 缓存级别
    int block_size{HJ_FILE_BLOCK_SIZE_DEFAULT};    // 文件块大小
    HJBlob block_status;        // 位图：1bit/块，1=已存在,0=不存在

    bool operator==(const HJDBFileInfo& other) const {
        return rid == other.rid &&
               url == other.url &&
               local_url == other.local_url &&
               status == other.status &&
               size == other.size &&
               total_length == other.total_length &&
               category == other.category &&
               create_time == other.create_time &&
               modify_time == other.modify_time &&
               use_count == other.use_count &&
               level == other.level &&
               block_size == other.block_size &&
               block_status.bytes == other.block_status.bytes;
    }

    // 位图辅助与进度同步
    int64_t blockCount() const {
        if (block_size <= 0) return 0;
        return (total_length + block_size - 1) / block_size;
    }
    static inline size_t byteIndex(size_t bitIndex) { return bitIndex >> 3; }
    static inline uint8_t bitMask(size_t bitIndex) { return static_cast<uint8_t>(1u << (bitIndex & 7)); }
    void ensureBitmap(size_t bitIndex) {
        const size_t needBytes = byteIndex(bitIndex) + 1;
        if (block_status.bytes.size() < needBytes) {
            block_status.bytes.resize(needBytes, '\0');
        }
    }
    int64_t blockLength(int64_t index) const {
        if (index < 0) return 0;
        const int64_t start = index * static_cast<int64_t>(block_size);
        if (start >= total_length) return 0;
        const int64_t end = (std::min)(total_length, start + static_cast<int64_t>(block_size));
        return end - start;
    }
    bool getBlockStatus(int index) const {
        if (index < 0) return false;
        const size_t bit = static_cast<size_t>(index);
        const size_t bIdx = byteIndex(bit);
        if (bIdx >= block_status.bytes.size()) return false;
        const auto b = static_cast<uint8_t>(block_status.bytes[bIdx]);
        return (b & bitMask(bit)) != 0;
    }
    void setBlockStatus(int index, bool present) {
        if (index < 0) return;
        const size_t bit = static_cast<size_t>(index);
        ensureBitmap(bit);
        auto &cell = block_status.bytes[byteIndex(bit)];
        auto v = static_cast<uint8_t>(cell);
        if (present) v |= bitMask(bit);
        else         v &= static_cast<uint8_t>(~bitMask(bit));
        cell = static_cast<char>(v);
    }
    void setBlockStatusAndUpdateSize(int index, bool present) {
        const bool was = getBlockStatus(index);
        if (was == present) return;
        setBlockStatus(index, present);
        const int64_t delta = blockLength(index);
        if (present) size = (std::min)(total_length, size + delta);
        else         size = (std::max)(static_cast<int64_t>(0), size - delta);
    }
};

NS_HJ_END

