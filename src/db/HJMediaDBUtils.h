//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJDBBlockStatus.h"
#include "HJFileUtil.h"
#include "HJMediaUtils.h"
#include <algorithm>
#include <optional>

NS_HJ_BEGIN
//***********************************************************************************//
#define HJ_DB_TABLE_MAIN_NAME "category"
#define HJ_DB_TABLE_GIFTS_NAME "gifts"
#define HJ_DB_TABLE_MEDIAS_NAME "medias"
//***********************************************************************************//
// 类别信息结构体
struct HJDBCategoryInfo
{
    std::string name{""};               // 主键, 类别名称
    int         type{0};                // 类型
    int64_t     max_size{0};            // 类别限制大小, 字节数
    int         file_count{0};          // 类别文件数量
    int64_t     total_size{0};          // 类别总存储占用空间大小
    std::string local_dir;              // 分类路径

    bool operator==(const HJDBCategoryInfo& other) const {
        return name == other.name &&
               type == other.type &&
               max_size == other.max_size &&
               file_count == other.file_count &&
               total_size == other.total_size &&
               local_dir == other.local_dir;
    }
    bool operator!=(const HJDBCategoryInfo& other) const {
        return !(*this == other);
    }

    static std::string makeCategoryName(const std::string& local_dir) {
        return HJ2STR(HJUtilitys::hash(HJUtilitys::trimmedDir(local_dir)));
    }
    /**
     * @brief 创建类别信息
     * @param local_dir   类别路径
     * @param max_size    类别限制大小, 字节数
     * @param name        类别名称
     * @return

     */
    static HJDBCategoryInfo makeCategoryInfo(const std::string& local_dir, const int64_t max_size, std::string name = "") {
        HJDBCategoryInfo info;
        info.name = name.empty() ? makeCategoryName(local_dir) : name;
        info.type = 0;
        info.max_size = max_size;   // * 1024 * 1024;
        info.local_dir = local_dir;
        return info;
    }
};



// 文件信息结构体
struct HJDBFileInfo
{
    std::string rid{""};            // 主键, 文件唯一标识
    std::string url{""};            // 文件网络URL
    std::string local_url{""};      // 文件本地URL
    int         status{0};          // 文件状态, 参考:HJFileStatus
    int64_t     size{0};            // 已完成字节数（用于进度）
    int64_t     total_length{0};    // 总字节数
    std::string category{""};       // 文件所属类别category.name
    int64_t     create_time{0};     // 创建时间
    int64_t     modify_time{0};     // 修改时间
    int         use_count{0};       // 使用次数
    int         level{0};           // 缓存级别
    int         block_size{32 * 1024 /*HJ_FILE_BLOCK_SIZE_DEFAULT*/};    // 文件块大小
    HJDBBlockStatus block_status;       // 位图：1bit/块，1=已存在,0=不存在
    std::vector<HJDBSpanInfo> spans;

    HJDBFileInfo() = default;
    HJDBFileInfo(const HJDBFileInfo& other) = default;
    HJDBFileInfo& operator=(const HJDBFileInfo& other) = default;

    HJDBFileInfo(const std::string& rid, const std::string& url, const std::string& local_url, const int64_t total_length, const int block_size)
        : rid(rid) , url(url), local_url(local_url), total_length(total_length), block_size(block_size)
    {
        status = (int)HJFileStatus::NEW;
        auto parent_dir = HJFileUtil::parentDir(local_url);
        category = HJDBCategoryInfo::makeCategoryName(parent_dir);
        create_time = HJCurrentSteadyMS();
        modify_time = create_time;
        use_count = 1;
        ensureBitmap(blockCount());
        spans.push_back({local_url});
    }
    HJDBFileInfo(const std::string& rid, const std::string& url, const std::string& local_url, const int status, const int64_t size, const int64_t total_length, const std::string& category, const int64_t create_time, const int64_t modify_time, const int use_count, const int level, const int block_size, const HJDBBlockStatus& block_status, const std::vector<HJDBSpanInfo>& spans)
        : rid(rid), url(url), local_url(local_url), status(status), size(size), total_length(total_length), category(category), create_time(create_time), modify_time(modify_time), use_count(use_count), level(level), block_size(block_size), block_status(block_status), spans(spans) {}
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
               block_status.bytes == other.block_status.bytes &&
               spans == other.spans;
    }
    bool operator!=(const HJDBFileInfo& other) const {
        return !(*this == other);
    }

    // 位图辅助与进度同步
    int64_t blockCount() const {
        if (block_size <= 0) return 0;
        return (total_length + block_size - 1) / block_size;
    }
    static inline size_t byteIndex(size_t bitIndex) { return bitIndex >> 3; }
    static inline uint8_t bitMask(size_t bitIndex) { return static_cast<uint8_t>(1u << (bitIndex & (size_t)7)); }
    void ensureBitmap(size_t bitIndex) {
        const size_t needBytes = byteIndex(bitIndex) + 1;
        if (block_status.bytes.size() < needBytes) {
            block_status.bytes.resize(needBytes, 0);
        }
    }
    void ensure() {
        ensureBitmap(blockCount());
    }

    int64_t blockLength(int64_t index) const {
        if (index < 0) return 0;
        const int64_t start = index * static_cast<int64_t>(block_size);
        if (start >= total_length) return 0;
        const int64_t end = (std::min)(total_length, start + static_cast<int64_t>(block_size));
        return end - start;
    }
    HJRange64i getBlockRange(int64_t block_index) const
    {
        const int64_t start = block_index * block_size;
        const int64_t end = (std::min)(start + block_size, total_length) - 1;
        return {start, end};
    }
    bool getBlockStatus(int index) const {
        if (index < 0) return false;
        const size_t bit = static_cast<size_t>(index);
        const size_t bIdx = byteIndex(bit);
        if (bIdx >= block_status.bytes.size()) return false;
        const uint8_t b = block_status.bytes[bIdx];
        return (b & bitMask(bit)) != 0;
    }
    void setBlockStatus(int index, bool present) {
        if (index < 0) return;
        const size_t bit = static_cast<size_t>(index);
        ensureBitmap(bit);
        uint8_t& cell = block_status.bytes[byteIndex(bit)];
        if (present) cell |= bitMask(bit);
        else         cell &= static_cast<uint8_t>(~bitMask(bit));
    }
    void setBlockStatusAndUpdateSize(int index, bool present) {
        const bool was = getBlockStatus(index);
        if (was == present) return;
        setBlockStatus(index, present);
        const int64_t delta = blockLength(index);
        if (present) size = (std::min)(total_length, size + delta);
        else         size = (std::max)(static_cast<int64_t>(0), size - delta);
    }

    /**
     * Recalculate size from bitmap to fix cumulative errors.
     * Should be called periodically or during data verification.
     */
    void recalculateSizeFromBitmap() {
        int64_t calculated_size = 0;
        const int64_t total_blocks = blockCount();
        for (int64_t i = 0; i < total_blocks; ++i) {
            if (getBlockStatus(static_cast<int>(i))) {
                calculated_size += blockLength(i);
            }
        }
        size = calculated_size;
    }

    /**
     * Check if all blocks are present (file is complete).
     */
    bool isComplete() const {
        if (total_length <= 0) return false;
        const int64_t total_blocks = blockCount();
        for (int64_t i = 0; i < total_blocks; ++i) {
            if (!getBlockStatus(static_cast<int>(i))) {
                return false;
            }
        }
        return true;
    }

    std::optional<HJDBSpanInfo> getSpanInfo(size_t index) const
    {
        for (const auto& span : spans) {
            int span_end = span.block_start_idx + span.block_cnt;
            if (static_cast<int>(index) >= span.block_start_idx && 
                static_cast<int>(index) < span_end) {
                return span;
            }
        }
        return std::nullopt;
    }
};

NS_HJ_END

