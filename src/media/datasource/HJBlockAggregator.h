//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaUtils.h"
#include <vector>

NS_HJ_BEGIN

/**
 * HJBlockAggregator - 块聚合器抽象接口 (Strategy Pattern)
 * 
 * 职责：将 Storage_Block 索引列表转换为 Fetch_Block 大小的下载范围
 */
class HJBlockAggregator : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJBlockAggregator);
    
    virtual ~HJBlockAggregator() = default;
    
    /**
     * @brief 聚合缺失块为下载范围
     * @param missing_blocks 缺失的 Storage_Block 索引列表（已排序）
     * @param storage_block_size Storage_Block 大小 (32KB)
     * @param fetch_block_size Fetch_Block 大小 (1MB)
     * @param total_file_size 文件总大小
     * @return 聚合后的下载范围列表
     */
    virtual std::vector<HJRange64i> aggregate(
        const std::vector<int64_t>& missing_blocks,
        int64_t storage_block_size,
        int64_t fetch_block_size,
        int64_t total_file_size) = 0;
    
    /**
     * @brief 验证配置有效性
     * @return true 如果 fetch_block_size 是 storage_block_size 的整数倍
     */
    static bool validateBlockSizes(int64_t storage_block_size, int64_t fetch_block_size) {
        if (storage_block_size <= 0 || fetch_block_size <= 0) return false;
        return (fetch_block_size % storage_block_size) == 0;
    }
    
    /**
     * @brief 计算一个 Fetch_Block 包含多少个 Storage_Block
     */
    static int64_t blocksPerFetch(int64_t storage_block_size, int64_t fetch_block_size) {
        if (storage_block_size <= 0) return 0;
        return fetch_block_size / storage_block_size;
    }
};

NS_HJ_END
