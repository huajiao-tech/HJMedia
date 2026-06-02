//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJBlockAggregator.h"

NS_HJ_BEGIN

/**
 * HJRangeAggregator - 连续块聚合器
 * 
 * 聚合策略：
 * 1. 扫描连续的 Storage_Block 索引
 * 2. 将连续块合并为不超过 Fetch_Block 大小的范围
 * 3. 非连续块产生独立的范围
 */
class HJRangeAggregator : public HJBlockAggregator
{
public:
    HJ_DECLARE_PUWTR(HJRangeAggregator);
    
    std::vector<HJRange64i> aggregate(
        const std::vector<int64_t>& missing_blocks,
        int64_t storage_block_size,
        int64_t fetch_block_size,
        int64_t total_file_size) override;

private:
    /**
     * @brief 将块索引转换为字节范围
     */
    HJRange64i blockIndexToRange(
        int64_t start_block,
        int64_t end_block,
        int64_t storage_block_size,
        int64_t total_file_size) const;
};

NS_HJ_END
