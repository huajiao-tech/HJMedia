//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRangeAggregator.h"

NS_HJ_BEGIN

std::vector<HJRange64i> HJRangeAggregator::aggregate(
    const std::vector<int64_t>& missing_blocks,
    int64_t storage_block_size,
    int64_t fetch_block_size,
    int64_t total_file_size)
{
    std::vector<HJRange64i> ranges;
    
    if (missing_blocks.empty() || storage_block_size <= 0 || fetch_block_size <= 0) {
        return ranges;
    }
    
    // 计算一个 Fetch_Block 可以包含多少个 Storage_Block
    int64_t blocks_per_fetch = blocksPerFetch(storage_block_size, fetch_block_size);
    if (blocks_per_fetch <= 0) {
        return ranges;
    }
    
    int64_t range_start_block = missing_blocks[0];
    int64_t range_end_block = missing_blocks[0];
    int64_t blocks_in_current_range = 1;
    
    for (size_t i = 1; i < missing_blocks.size(); ++i) {
        int64_t current_block = missing_blocks[i];
        
        // 检查是否连续且不超过 Fetch_Block 大小限制
        bool is_consecutive = (current_block == range_end_block + 1);
        bool within_fetch_limit = (blocks_in_current_range < blocks_per_fetch);
        
        if (is_consecutive && within_fetch_limit) {
            // 扩展当前范围
            range_end_block = current_block;
            blocks_in_current_range++;
        } else {
            // 保存当前范围并开始新范围
            ranges.push_back(blockIndexToRange(range_start_block, range_end_block, 
                                               storage_block_size, total_file_size));
            range_start_block = current_block;
            range_end_block = current_block;
            blocks_in_current_range = 1;
        }
    }
    
    // 保存最后一个范围
    ranges.push_back(blockIndexToRange(range_start_block, range_end_block, 
                                       storage_block_size, total_file_size));
    
    return ranges;
}

HJRange64i HJRangeAggregator::blockIndexToRange(
    int64_t start_block,
    int64_t end_block,
    int64_t storage_block_size,
    int64_t total_file_size) const
{
    HJRange64i range;
    range.begin = start_block * storage_block_size;
    
    // 计算结束位置，考虑文件末尾不完整块的情况
    int64_t theoretical_end = (end_block + 1) * storage_block_size - 1;
    range.end = (total_file_size > 0) ? HJ_MIN(theoretical_end, total_file_size - 1) : theoretical_end;
    
    return range;
}

NS_HJ_END
