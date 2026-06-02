//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXIOBlobFile.h"
#include "HJFLog.h"
#include <algorithm>
#include <cmath>

NS_HJ_BEGIN

//***********************************************************************************//
// 构造与析构
//***********************************************************************************//
HJXIOBlobFile::HJXIOBlobFile(HJUrl::Ptr url, HJListener listener)
    : m_file(std::make_unique<HJXIOFile>())
    , m_listener(listener) 
{
    if (url) {
        open(url);
    }
}

HJXIOBlobFile::~HJXIOBlobFile() {
    close();
}

//***********************************************************************************//
// 打开文件
//***********************************************************************************//
int HJXIOBlobFile::open(HJUrl::Ptr url) {
    HJ_AUTO_LOCK(m_mutex);
    if (!url) {
        return HJErrInvalidParams;
    }
    int res = HJXIOBase::open(url);
	if (HJ_OK != res) {
		return res;
	}
    res = m_file->open(url);
    if (res != 0) {
        return res;
    }
    notify(HJMakeNotification((size_t)HJFileStatus::NEW, url->getUrl()));
    //
    m_pos = 0;
    if(url->haveValue("size")) {
        m_size = url->getInt64("size");
    } else {
        m_size = m_file->size();
    }
    m_blocks.clear();
    m_block_status.clear();
    m_completed_block_count = 0;
   
    ensureBlockStatusCapacity(m_size);
    
    return res;
}

//***********************************************************************************//
// 关闭文件
//***********************************************************************************//
void HJXIOBlobFile::close() {
    flush(); // 关闭前强制刷盘
    HJ_AUTO_LOCK(m_mutex);
    if (m_file) {
        m_file->close();
    }
    m_blocks.clear();
    m_block_status.clear(); // 或许不应清空，视复用需求而定
}

//***********************************************************************************//
// 读取数据
//***********************************************************************************//
int HJXIOBlobFile::read(void* buffer, size_t cnt) {
    if (!buffer || cnt == 0) return 0;
    
    HJ_AUTO_LOCK(m_mutex);
    
    size_t total_read = 0;
    uint8_t* ptr = static_cast<uint8_t*>(buffer);
    
    while (total_read < cnt) {
        if (m_pos >= m_size) break; // EOF
        
        size_t block_idx = getBlockIndex(m_pos);
        auto it = m_blocks.find(block_idx);
        
        size_t current_read = 0;
        size_t block_offset = m_pos % kBlockSize;
        size_t to_read = std::min(cnt - total_read, m_size - m_pos);
        // 进一步限制在当前块内
        size_t block_remaining = kBlockSize - block_offset;
        to_read = std::min(to_read, block_remaining);
        
        if (to_read == 0) break;
        
        if (it != m_blocks.end() && it->second->isComplete()) {
            // 缓存命中，从内存块读取
            // HJBlockData 没有 read 接口，直接访问 data()
            const uint8_t* block_data = it->second->data();
            std::copy(block_data + block_offset, block_data + block_offset + to_read, ptr + total_read);
            current_read = to_read;
        } else {
            // 缓存未命中，直接从物理文件读取
            // 为了保持 m_pos 不受 m_file 内部指针影响，使用 pread 语义封装
            int ret = readFromFile(m_pos, ptr + total_read, to_read);
            if (ret <= 0) break; 
            current_read = static_cast<size_t>(ret);
        }
        
        total_read += current_read;
        m_pos += current_read;
    }
    
    return static_cast<int>(total_read);
}

//***********************************************************************************//
// 写入数据
//***********************************************************************************//
int HJXIOBlobFile::write(const void* buffer, size_t cnt) {
    if (!buffer || cnt == 0) return 0;
    
    HJ_AUTO_LOCK(m_mutex);
    
    const uint8_t* ptr = static_cast<const uint8_t*>(buffer);
    size_t total_written = 0;
    const size_t remaining = (m_pos < m_size) ? (m_size - m_pos) : 0;
    if (remaining == 0) {
        return 0;
    }
    const size_t target_cnt = std::min(cnt, remaining);
    
    // 预扩容状态追踪器
    ensureBlockStatusCapacity(m_size);
    
    while (total_written < target_cnt) {
        // 获取或创建当前块
        auto block = getOrCreateBlock(m_pos);
        if (!block) {
            HJFLoge("write: create block failed, pos:{}, file size:{}", m_pos, m_size);
            return -1; // 创建失败
        }
        
        // 计算当前块内写入位置和长度
        size_t block_offset = block->toLocalOffset(m_pos);
        if (block_offset == SIZE_MAX) break;
        size_t block_valid_size = block->getValidSize();
        if (block_offset >= block_valid_size) break;
        size_t to_write = std::min(target_cnt - total_written, block_valid_size - block_offset);
        if (to_write == 0) break;
        
        // 执行写入
        size_t written = block->write(block_offset, ptr + total_written, to_write);
        if (written == 0) {
            HJFLogw("write: block write failed");
            break; // 应该不会发生，除非逻辑错误
        }
        
        // 检查块是否完整
        if (block->isComplete()) {
            // 完整则自动刷盘
            if (flushBlock(block) == 0) {
                // 更新全局状态追踪
                size_t block_idx = getBlockIndex(block->getGlobalStartOffset());
                if (block_idx < m_block_status.size() && !m_block_status[block_idx]) {
                    m_block_status[block_idx] = true;
                    //
                    addCompletedBlockCount(block);
                }
                
                // 刷盘且状态更新后，可以选择从 m_blocks 中移除以节省内存
                if(m_clearupAfterWrited) {
                    m_blocks.erase(block_idx);
                }
            }
        }
        
        total_written += written;
        m_pos += written;
        
        // 更新文件逻辑大小
        if (m_pos > m_size) {
            m_size = m_pos;
        }
    }
    
    return static_cast<int>(total_written);
}

//***********************************************************************************//
// 刷盘指定块
//***********************************************************************************//
int HJXIOBlobFile::flushBlock(const HJBlockData::Ptr& block) {
    if (!block) return -1;
    
    // 检查 buffer 是否已分配（懒加载模式下可能未分配）
    const uint8_t* data = block->data();
    if (!data) {
        HJFLogw("flushBlock: block buffer not allocated, skip flush");
        return -1;
    }
    
    // 使用 writeToFile 进行物理写入
    // 注意：只写入有效数据部分
    return writeToFile(block->getGlobalStartOffset(), 
                       data, 
                       block->getValidSize());
}

//***********************************************************************************//
// 获取或创建块
//***********************************************************************************//
HJBlockData::Ptr HJXIOBlobFile::getOrCreateBlock(size_t global_offset) {
    size_t block_idx = getBlockIndex(global_offset);
    
    // 检查是否已存在
    auto it = m_blocks.find(block_idx);
    if (it != m_blocks.end()) {
        return it->second;
    }
    
    // 计算块属性
    size_t block_start_offset = block_idx * kBlockSize;
    
    // 计算有效大小：对于最后一块，可能小于 kBlockSize
    // 如果是追加写入，且 m_pos + len 超过了当前文件大小，如何确定 valid_size？
    // HJBlockData 的 valid_size 一旦构造即固定。
    // 这里我们假设标准分块模式：除了逻辑上的最后一块外，其余均为 32KB。
    // 但如果用户 seek 到很后面写，中间可能有空洞。
    // 简化处理：默认 32KB。只有当明确知道是文件末尾且需截断时才使用小尺寸。
    // 实际上，为了支持随机写，我们统一按 32KB 创建，只有 truly EOF block 特殊处理?
    // 根据需求 "默认大小32KB... 文件末尾块大小不够32KB"，
    // 我们暂时统一按 32KB 创建。如果是写入操作导致文件增长，HJBlockData 会处理局部的 write。
    // 关键是：如果最后一块只有 10KB 数据，我们构造时传入 32KB 还是 10KB？
    // 如果传入 10KB，后续追加就写不进去了。
    // 所以构造时应尽量按 MAX (32KB) 创建。
    // 这里的 valid_size 更多是用于 "读取已知大小文件" 时限制末尾块。
    // 对于正在写入的文件，通常都是 32KB 容量。
    // *修正*：为了兼容末尾截止校验，如果当前写入操作是在已知文件大小边界内，
    // 且是最后一块，可以设为实际大小。但在 write 模式下，通常期望能写满。
    // 因此，总是以 kBlockSize 创建，除非是只读模式下的切片。
    // 鉴于 write 接口特性，我们这里统一使用 kBlockSize 初始化，
    // 以支持随时追加填满该块。
    if(block_start_offset >= m_size) {
        return nullptr;
    }
    size_t valid_size = HJ_MIN(kBlockSize, m_size - block_start_offset);
    if (valid_size == 0) {
        return nullptr;
    }
    // 特殊情况：如果是只读打开且预加载，可能会用到非 full size，但这是 write 场景。
    
    auto block = std::make_shared<HJBlockData>(block_start_offset, valid_size);
    block->setID(block_idx);

    m_blocks[block_idx] = block;
    return block;
}

//***********************************************************************************//
// 其他接口实现
//***********************************************************************************//
int HJXIOBlobFile::seek(int64_t offset, int whence) {
    HJ_AUTO_LOCK(m_mutex);
    int64_t new_pos = m_pos;
    
    switch (whence) {
        case std::ios::beg: new_pos = offset; break;
        case std::ios::cur: new_pos = m_pos + offset; break;
        case std::ios::end: new_pos = m_size + offset; break;
        default: return -1;
    }
    
    if (new_pos < 0) return -1;
    m_pos = static_cast<size_t>(new_pos);
    return 0; // standard seek returns 0 on success
}

int HJXIOBlobFile::flush() {
    HJ_AUTO_LOCK(m_mutex);
    
    // 遍历所有活跃块，仅刷完整且未落盘的块。
    
    for (const auto& pair : m_blocks) {
        const size_t block_idx = pair.first;
        const auto& block = pair.second;
        if (block_idx < m_block_status.size() && m_block_status[block_idx]) {
            continue;
        }
        if (!block->isComplete()) {
            continue;
        }
        // 如果块有数据 (written > 0) 尝试写入
        // 这里简化直接全量 flushBlock（内部 writeToFile 可处理）
        // 但 flushBlock 是写整块 valid_size。如果块没写满（written < valid_size），
        // 里面的 0 也会被写入覆盖文件原有内容（如果存在）？
        //这是一个潜在问题。HJBlockData::data() 返回完整 buffer。
        // 如果是追加写，没问题。如果是覆盖写，未写部分是 0，可能会破坏原文件数据
        // 如果原文件该位置有数据。
        // *解决方案*：HJBlockData 目前是基于内存缓冲的全覆盖。
        // 如果要支持 "部分修改而不破坏未修改部分"，需要读入-修改-写出 (RMW)。
        // 鉴于这是一个 BlobFile，通常是新文件生成或流式下载，暂按全量写处理。
        
        if (block->getWrittenCount() > 0) {
            if (flushBlock(block) == 0 && block_idx < m_block_status.size()) {
                if (!m_block_status[block_idx]) {
                    m_block_status[block_idx] = true;
                    //
                    addCompletedBlockCount(block);
                }
            }
        }
    }
    
    if (m_file) m_file->flush();
    return 0;
}

int64_t HJXIOBlobFile::tell() {
    return static_cast<int64_t>(m_pos);
}

int64_t HJXIOBlobFile::size() {
    // 逻辑大小（包含还在内存中缓冲的部分）
    HJ_AUTO_LOCK(m_mutex);
    return static_cast<int64_t>(m_size);
}

bool HJXIOBlobFile::eof() {
    HJ_AUTO_LOCK(m_mutex);
    return m_pos >= m_size;
}

void HJXIOBlobFile::setSize(const int64_t size) {
    HJ_AUTO_LOCK(m_mutex);
    HJXIOBase::setSize(size);
    ensureBlockStatusCapacity(static_cast<size_t>(size));
}

//***********************************************************************************//
// 辅助方法
//***********************************************************************************//
void HJXIOBlobFile::ensureBlockStatusCapacity(size_t file_size) {
    size_t required_blocks = (file_size + kBlockSize - 1) / kBlockSize;
    if (m_block_status.size() < required_blocks) {
        m_block_status.resize(required_blocks, false);
    }
}

size_t HJXIOBlobFile::getBlockIndex(size_t global_offset) const {
    return global_offset / kBlockSize;
}

int HJXIOBlobFile::writeToFile(int64_t offset, const void* data, size_t len) {
    if (!m_file) return -1;
    
    // 保存当前物理文件指针
    int64_t old_pos = m_file->tell();
    
    // seek 并写
    if (m_file->seek(offset, std::ios::beg) != 0) return -1;
    int ret = m_file->write(data, len);
    
    // 恢复指针（可选，如果 m_file 仅在此处使用则不必要，
    // 但为了 readFromFile 互斥安全，最好恢复或每次都 seek）
    m_file->seek(old_pos, std::ios::beg);
    
    return (ret == static_cast<int>(len)) ? 0 : -1;
}

int HJXIOBlobFile::readFromFile(int64_t offset, void* data, size_t len) {
    if (!m_file) return -1;
    
    int64_t old_pos = m_file->tell();
    
    if (m_file->seek(offset, std::ios::beg) != 0) return -1;
    int ret = m_file->read(data, len);
    
    m_file->seek(old_pos, std::ios::beg);
    
    return ret;
}

bool HJXIOBlobFile::isComplete() const {
    HJ_AUTO_LOCK(m_mutex);
    // 检查所有已知块的状态
    // 通过计数器直接判断，避免遍历
    // 注意：m_block_status.size() 代表当前文件大小对应的总块数
    if (m_block_status.empty()) return false; // 还没分配？或者空文件？如果 size=0，status empty，isComplete?
    
    // 如果文件大小为0，size=0 -> required=0 -> vector empty. 
    // 此时应该算 complete 吗？
    // 根据 loop 逻辑：for(bool) {} return true. 空循环返回 true.
    // 所以 empty 也是 true.
    if (m_block_status.empty() && m_size == 0) return true;
    
    return m_completed_block_count == m_block_status.size();
}

bool HJXIOBlobFile::isBlockComplete(size_t index) const {
    HJ_AUTO_LOCK(m_mutex);
    if (index >= m_block_status.size()) return false;
    return m_block_status[index];
}

NS_HJ_END
