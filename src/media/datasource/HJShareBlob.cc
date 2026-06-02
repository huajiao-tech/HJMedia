//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJShareBlob.h"
#include "HJFLog.h"
#include <algorithm>

NS_HJ_BEGIN
//***********************************************************************************//

HJShareBlob::HJShareBlob(const HJDBFileInfo& info, HJListener listener)
    : m_db_info(info)
    , m_listener(listener)
{
}

HJShareBlob::~HJShareBlob()
{
    done();
}

//***********************************************************************************//
// 初始化接口
//***********************************************************************************//
int HJShareBlob::init()
{
    HJ_AUTO_LOCK(m_mutex);
    
    for (const auto& span : m_db_info.spans) {
        if (!span.local_url.empty()) {
            auto it = m_write_urls.find(span.local_url);
            if (it == m_write_urls.end()) {
                HJFLogi("write url:{}", span.local_url);
                m_write_urls.emplace(span.local_url, HJSourceWriterUrl(span.local_url, false));
            }
        }
    }
    return HJ_OK;
}

int HJShareBlob::done()
{
    HJ_AUTO_LOCK(m_mutex);
    m_blocks.clear();
    return HJ_OK;
}

//***********************************************************************************//
// 核心块获取接口
//***********************************************************************************//
HJBlockData::Ptr HJShareBlob::getBlock(size_t block_idx)
{
    HJ_AUTO_LOCK(m_mutex);
    
    auto block = getOrCreateBlock(block_idx);
    if (!block) {
        HJFLoge("getBlock failed, block {} not found", block_idx);
        return nullptr;
    }
    
    HJBlockData::BlockStatus status = block->getStatus();
    switch (status) {
        case HJBlockData::BlockStatus::CACHED:
            // 已缓存，直接返回，不改变状态
            return block;
        case HJBlockData::BlockStatus::HOLE:
            // 空洞，锁定并返回
            block->setStatus(HJBlockData::BlockStatus::LOCKED);
            return block;
            
        case HJBlockData::BlockStatus::LOCKED:
        case HJBlockData::BlockStatus::ALONE: {
            // 已锁定，复制一份返回 ALONE 副本
            auto alone_block = std::make_shared<HJBlockData>(
                block->getGlobalStartOffset(), 
                block->getValidSize()
            );
            alone_block->setID(block->getID());
            alone_block->setStatus(HJBlockData::BlockStatus::ALONE);
            // ALONE 块不加入 m_blocks，由调用者管理
            return alone_block;
        }
    }
    
    return nullptr;
}


//***********************************************************************************//
// 状态转换接口
//***********************************************************************************//
int HJShareBlob::commitBlock(size_t block_idx, const std::string& local_url)
{
    if(local_url.empty()) {
        return HJErrInvalidParams;
    }
    HJ_AUTO_LOCK(m_mutex);
    
    auto it = m_blocks.find(block_idx);
    if (it == m_blocks.end()) {
        return HJErrNotFind;
    }
    
    auto& block = it->second;
    if (block->getStatus() != HJBlockData::BlockStatus::LOCKED) {
        return HJErrNotFind;  // 只能从 LOCKED 转换
    }
    HJFLogi("commit block:{}, url:{}", block_idx, local_url);
    
    block->setStatus(HJBlockData::BlockStatus::CACHED);
    block->setLocalUrl(local_url);
    block->clearBuffer();
    
    // 更新 db_info 的 bitmap
    m_db_info.setBlockStatusAndUpdateSize(static_cast<int>(block_idx), true);
    
    // 更新 spans 信息，合并连续块
    mergeSpans(block_idx, local_url);
    
    m_dirty_block_count++;
    if (m_dirty_block_count >= kPersistThreshold) {
        int percent = m_db_info.size * 100 / m_db_info.total_length;
        auto ntfy = HJMakeNotification(static_cast<int>(HJFileStatus::PENDING), percent, m_db_info.rid);
        (*ntfy)["db_info"] = m_db_info;
        notify(ntfy);
        m_dirty_block_count = 0;
        HJFLogi("file download block_idx:{} progress:{}, url:{}, rid:{}, current local url:{}", block_idx, percent, m_db_info.url, m_db_info.rid, local_url);
    }
    if (m_db_info.isComplete()) {
        m_db_info.status = static_cast<int>(HJFileStatus::COMPLETED);
        int percent = 100;
        auto ntfy = HJMakeNotification(static_cast<int>(HJFileStatus::COMPLETED), percent, m_db_info.rid);
        (*ntfy)["db_info"] = m_db_info;
        notify(ntfy);
        HJFLogi("file download block_idx:{} completed, url:{}, rid:{}, current local url:{}", block_idx, m_db_info.url, m_db_info.rid, local_url);
    }
    
    return HJ_OK;
}

int HJShareBlob::releaseBlock(size_t block_idx)
{
    HJ_AUTO_LOCK(m_mutex);
    
    auto it = m_blocks.find(block_idx);
    if (it == m_blocks.end()) {
        return HJErrNotFind;
    }
    
    auto& block = it->second;
    if (block->getStatus() != HJBlockData::BlockStatus::LOCKED) {
        return HJ_OK;  // 只能从 LOCKED 转换
    }
    if(!block->isComplete()) {
        block->setStatus(HJBlockData::BlockStatus::HOLE);
        // block->clearBuffer();  // 释放可能已部分下载的数据
    } else {
        block->setStatus(HJBlockData::BlockStatus::CACHED);
    }
    
    return HJ_OK;
}

//***********************************************************************************//
// 查询接口
//***********************************************************************************//
size_t HJShareBlob::blockCount() const
{
    return static_cast<size_t>(m_db_info.blockCount());
}

HJBlockData::BlockStatus HJShareBlob::getBlockStatus(size_t block_idx) const
{
    HJ_AUTO_LOCK(m_mutex);
    
    auto it = m_blocks.find(block_idx);
    if (it == m_blocks.end()) {
        // 根据 db_info 的 bitmap 判断
        if (m_db_info.getBlockStatus(static_cast<int>(block_idx))) {
            return HJBlockData::BlockStatus::CACHED;
        }
        return HJBlockData::BlockStatus::HOLE;
    }
    
    return it->second->getStatus();
}

std::string HJShareBlob::getBlockLocalUrl(size_t block_idx) const
{
    HJ_AUTO_LOCK(m_mutex);
    
    auto it = m_blocks.find(block_idx);
    if (it != m_blocks.end()) {
        return it->second->getLocalUrl();
    }
    
    // 从 spans 中查找
    return findLocalUrlInSpans(block_idx);
}

std::string HJShareBlob::lockWriteUrl()
{
    HJ_AUTO_LOCK(m_mutex);
    std::string write_url = "";
    for (auto& xurl : m_write_urls) {
        if (!xurl.second.is_used) {
            xurl.second.is_used = true;
            write_url = xurl.second.local_url;
            break;
        }
    }
    
    if (write_url.empty()) {
        write_url = m_write_urls.empty() ? m_db_info.local_url : HJMediaUtils::makeHJTDFile(m_db_info.local_url, m_write_urls.size());
        m_write_urls.emplace(write_url, HJSourceWriterUrl(write_url, true));
    }
    return write_url;
}

void HJShareBlob::unlockWriteUrl(const std::string& write_url)
{
    if (write_url.empty()) {
        return;
    }
    
    HJ_AUTO_LOCK(m_mutex);
    auto it = m_write_urls.find(write_url);
    if (it != m_write_urls.end()) {
        it->second.is_used = false;
    }
}

std::optional<HJDBSpanInfo> HJShareBlob::findSpanForBlock(size_t block_idx) const
{
    for (const auto& span : m_db_info.spans) {
        int span_end = span.block_start_idx + span.block_cnt;
        if (static_cast<int>(block_idx) >= span.block_start_idx && 
            static_cast<int>(block_idx) < span_end) {
            return span;
        }
    }
    return std::nullopt;
}

HJBlobMissingInfo HJShareBlob::getMissingInfo()
{
    HJ_AUTO_LOCK(m_mutex);
    HJBlobMissingInfo info;
    info.holeRanges = getHoleRanges();
    info.incompleteBlocks = getIncompleteBlocks();
    info.validSize = m_db_info.size;
    info.totalLength = m_db_info.total_length;
    info.blockSize = m_db_info.block_size;
    info.status = m_db_info.status;
    return info;
}

std::vector<HJRange64i> HJShareBlob::getHoleRanges()
{
    std::vector<HJRange64i> ranges;
    int64_t count = m_db_info.blockCount();
    
    for (int64_t i = 0; i < count; ++i) {
        // quiet: false means hole
        if (!m_db_info.getBlockStatus(static_cast<int>(i))) {
            HJRange64i block_range = m_db_info.getBlockRange(i);
            
            if (ranges.empty()) {
                ranges.push_back(block_range);
            } else {
                auto& last = ranges.back();
                // Check if contiguous
                if (last.end + 1 == block_range.begin) {
                    last.end = block_range.end;
                } else {
                    ranges.push_back(block_range);
                }
            }
        }
    }
    return ranges;
}

//***********************************************************************************//
// 辅助方法
//***********************************************************************************//
HJBlockData::Ptr HJShareBlob::getOrCreateBlock(size_t block_idx)
{
    auto it = m_blocks.find(block_idx);
    if (it != m_blocks.end()) {
        return it->second;
    }
    
    size_t block_start_offset = block_idx * m_db_info.block_size;
    if (block_start_offset >= static_cast<size_t>(m_db_info.total_length)) {
        return nullptr;
    }
    
    size_t valid_size = (std::min)(static_cast<size_t>(m_db_info.block_size), 
                                   static_cast<size_t>(m_db_info.total_length) - block_start_offset);
    if (valid_size == 0) {
        return nullptr;
    }
    
    auto block = std::make_shared<HJBlockData>(block_start_offset, valid_size);
    block->setID(block_idx);

    // 延时初始化状态
    if (m_db_info.getBlockStatus(static_cast<int>(block_idx))) {
        block->setStatus(HJBlockData::BlockStatus::CACHED);
        // 从 spans 查找对应的 local_url
        auto local_url = findLocalUrlInSpans(block_idx);
        block->setLocalUrl(local_url);
    } else {
        block->setStatus(HJBlockData::BlockStatus::HOLE);
    }

    m_blocks[block_idx] = block;

    return block;
}

std::string HJShareBlob::findLocalUrlInSpans(size_t block_idx) const
{
    for (const auto& span : m_db_info.spans) {
        int span_end = span.block_start_idx + span.block_cnt;
        if (static_cast<int>(block_idx) >= span.block_start_idx && 
            static_cast<int>(block_idx) < span_end) {
            return span.local_url;
        }
    }
    return "";
}

void HJShareBlob::mergeSpans(size_t block_idx, const std::string& local_url)
{
    // 查找可合并的相邻 span
    HJDBSpanInfo* prev_span = nullptr;
    HJDBSpanInfo* next_span = nullptr;
    int insert_pos = -1;
    
    for (size_t i = 0; i < m_db_info.spans.size(); ++i) {
        auto& span = m_db_info.spans[i];
        int span_end = span.block_start_idx + span.block_cnt;
        
        // 检查是否可以向前合并（当前块紧跟在某 span 后面）
        if (span_end == static_cast<int>(block_idx) && span.local_url == local_url) {
            prev_span = &span;
        }
        // 检查是否可以向后合并（当前块在某 span 前面）
        if (span.block_start_idx == static_cast<int>(block_idx) + 1 && span.local_url == local_url) {
            next_span = &span;
        }
        // 记录插入位置
        if (span.block_start_idx > static_cast<int>(block_idx) && insert_pos < 0) {
            insert_pos = static_cast<int>(i);
        }
    }
    
    if (prev_span && next_span) {
        // 合并两个 span
        prev_span->block_cnt += 1 + next_span->block_cnt;
        // 移除 next_span
        auto it = std::find_if(m_db_info.spans.begin(), m_db_info.spans.end(),
            [next_span](const HJDBSpanInfo& s) { return &s == next_span; });
        if (it != m_db_info.spans.end()) {
            m_db_info.spans.erase(it);
        }
    } else if (prev_span) {
        // 向前扩展
        prev_span->block_cnt += 1;
    } else if (next_span) {
        // 向后扩展
        next_span->block_start_idx = static_cast<int>(block_idx);
        next_span->block_cnt += 1;
    } else {
        // 创建新 span
        HJDBSpanInfo new_span;
        new_span.block_start_idx = static_cast<int>(block_idx);
        new_span.block_cnt = 1;
        new_span.local_url = local_url;
        
        if (insert_pos >= 0) {
            m_db_info.spans.insert(m_db_info.spans.begin() + insert_pos, new_span);
        } else {
            m_db_info.spans.push_back(new_span);
        }
    }
}

NS_HJ_END