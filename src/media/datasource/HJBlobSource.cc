//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBlobSource.h"
#include "HJRangeAggregator.h"
#include "HJFileUtil.h"
#include "HJLocalFileManager.h"
#include "HJDataSourceKit.h"
#include "HJMediaUtils.h"
#include "HJBuffer.h"
#include "HJFLog.h"
#include "HJException.h"
#include "HJFFUtils.h"
#include <algorithm>

NS_HJ_BEGIN
//***********************************************************************************//
HJBlobSource::HJBlobSource(const HJXIOFileInfo& info, HJListener listener)
    : m_info(info)
    , m_listener(listener)
{
    auto fileManager = HJDataSourceKit::getInstance()->getFileManager();
    if (!fileManager) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, "file manager is null");
    }
    m_fileManager = fileManager;
}

HJBlobSource::~HJBlobSource()
{
    close();
}

int HJBlobSource::open()
{
    if (m_info.url.empty() || m_info.rid.empty() || m_info.local_url.empty()) {
        HJFLoge("error, url or rid or local_url is empty");
        return HJErrInvalidParams;
    }
    HJFLogi("entry, url: {}, rid: {}, local_url: {}", m_info.url, m_info.rid, m_info.local_url);

    int res = HJ_OK;
    do {
        m_local_rid = m_info.tmp_rid.empty() ? m_info.rid : m_info.tmp_rid;
        m_local_url = m_info.tmp_url.empty() ? m_info.local_url : m_info.tmp_url;
        auto fileManager = HJLockWtr(m_fileManager);
        if (!fileManager) {
            HJFLoge("error, file manager is null");
            return HJErrNotAlready;
        }
        auto db_file = fileManager->getDBFile(m_local_rid, m_info.category);
        if(!db_file.has_value()) {
            HJFLoge("error, db file not found, rid: {}, category: {}", m_local_rid, m_info.category);
            return HJErrNotFind;
        }
        m_db_info = db_file.value();
        if (m_db_info.block_size != static_cast<int>(HJBlockData::kBlockSize)) {
            HJFLoge("error, block_size mismatch, db: {}, expect: {}", m_db_info.block_size, HJBlockData::kBlockSize);
            return HJErrInvalidData;
        }

        // 如果文件未完成，需要打开远程文件和写入文件
        if(m_db_info.status != static_cast<int>(HJFileStatus::COMPLETED)) 
        {
            auto remote_murl = HJCreates<HJUrl>(m_info.url, HJ_XIO_READ);
            m_remoteFile = HJCreateu<HJXIOContext>();
            res = m_remoteFile->open(remote_murl);
            if (HJ_OK != res) {
                HJFLoge("error, failed to open url: {}", m_info.url);
                break;
            }
            auto file_total_length = m_remoteFile->size();
            if (file_total_length <= 0) {
                HJFLoge("error, file size is invalid");
                res = HJErrInvalidData;
                break;
            }

            if(m_db_info.total_length != file_total_length) {
                m_db_info.total_length = file_total_length;
                m_db_info.ensure();
            }

            HJXIOMode fmode = HJ_XIO_RWONLY;
            if(!HJFileUtil::isFileExist(m_local_url)) {
                fmode = HJ_XIO_WRITE;
            }
            auto write_murl = HJCreates<HJUrl>(m_local_url, fmode);
            m_writeFile = HJCreateu<HJXIOFile>();
            res = m_writeFile->open(write_murl);
            if (HJ_OK != res) {
                HJFLoge("error, failed to open local url for write: {}", m_local_url);
                break;
            }
        }

        // 打开读取文件
        auto read_murl = HJCreates<HJUrl>(m_local_url, HJ_XIO_READ);
        m_readFile = HJCreateu<HJXIOFile>();
        res = m_readFile->open(read_murl);
        if (HJ_OK != res) {
            HJFLoge("error, failed to open local url: {}", m_local_url);
            break;
        }

    } while (false);
    
    // 如果打开失败，清理已打开的资源
    if (res != HJ_OK) {
        if (m_remoteFile) {
            m_remoteFile->close();
            m_remoteFile.reset();
        }
        if (m_writeFile) {
            m_writeFile->close();
            m_writeFile.reset();
        }
        if (m_readFile) {
            m_readFile->close();
            m_readFile.reset();
        }
    }

    return res;
}

int HJBlobSource::read(void* buffer, size_t cnt)
{
    if (!buffer || cnt == 0) {
        return 0;
    }

    size_t total_read = 0;
    uint8_t* ptr = static_cast<uint8_t*>(buffer);

    while (total_read < cnt) 
    {
        // 检查是否到达文件末尾
        if (m_pos >= m_db_info.total_length) {
            break; // EOF
        }

        // 计算当前块信息
        size_t block_idx = getBlockIndex(static_cast<size_t>(m_pos));
        size_t block_offset = getBlockOffset(static_cast<size_t>(m_pos));
        
        // 计算本次可读取的字节数（不超过块边界）
        size_t bytes_remaining_in_block = m_db_info.block_size - block_offset;
        size_t bytes_to_read = (std::min)(cnt - total_read, bytes_remaining_in_block);
        
        // 确保不超过文件末尾
        if (m_pos + static_cast<int64_t>(bytes_to_read) > m_db_info.total_length) {
            bytes_to_read = static_cast<size_t>(m_db_info.total_length - m_pos);
        }

        int bytes_read = 0;

        // 选择读取源
        if (isBlockWriting(block_idx)) {
            // 块正在写入，从缓存读取
            bytes_read = readFromBlockCache(ptr + total_read, block_idx, block_offset, bytes_to_read);
        } else if (m_db_info.getBlockStatus(static_cast<int>(block_idx))) {
            // 块存在于本地，从本地文件读取
            bytes_read = readFromLocal(ptr + total_read, block_idx, block_offset, bytes_to_read);
        } else {
            // 块不存在，从远程读取
            bytes_read = readFromRemote(ptr + total_read, block_idx, block_offset, bytes_to_read);
        }

        if (bytes_read <= 0) {
            // 读取失败或无数据
            if (total_read == 0) {
                return bytes_read; // 返回错误码
            }
            break; // 返回已读取的数据
        }

        total_read += bytes_read;
        m_pos += bytes_read;
    }

    return static_cast<int>(total_read);
}

int HJBlobSource::readFromLocal(void* buffer, size_t block_idx, size_t offset, size_t cnt)
{
    if (!m_readFile) {
        HJFLoge("error, read file is null");
        return HJErrNotAlready;
    }

    int64_t file_pos = static_cast<int64_t>(block_idx) * m_db_info.block_size + offset;
    if(file_pos != m_readFile->tell()) 
    {
        HJFLogw("warning, read file pos mismatch, expected: {}, actual: {}", file_pos, m_readFile->tell());
        int seek_res = m_readFile->seek(file_pos, std::ios::beg);
        if (seek_res != 0) {
            HJFLoge("error, failed to seek read file to pos: {}", file_pos);
            return HJErrIO;
        }
    }
    int bytes_read = m_readFile->read(buffer, cnt);
    if (bytes_read < 0) {
        HJFLoge("error, failed to read from local file, pos: {}, cnt: {}", file_pos, cnt);
        return HJErrIO;
    }

    return bytes_read;
}

int HJBlobSource::readFromRemote(void* buffer, size_t block_idx, size_t offset, size_t cnt)
{
    if (!m_remoteFile) {
        HJFLoge("error, remote file is null");
        return HJErrNotAlready;
    }

    // 获取或创建块缓存
    size_t block_start = block_idx * m_db_info.block_size;
    auto block = getOrCreateBlock(block_start);
    if (!block) {
        HJFLoge("error, failed to create block for idx: {}", block_idx);
        return HJErrInvalidData;
    }

    int64_t file_pos = static_cast<int64_t>(block_start) + offset;
    if(file_pos != m_remoteFile->tell()) 
    {
        HJFLogw("warning, remote file pos mismatch, expected: {}, actual: {}", file_pos, m_remoteFile->tell());    
        int seek_res = m_remoteFile->seek(file_pos, std::ios::beg);
        if (seek_res != HJ_OK) {
            HJFLoge("error, failed to seek remote file to pos: {}, res: {}, err: {}", file_pos, seek_res, HJ_AVErr2Str(seek_res));
            return HJErrNetUnkown;
        }
    }

    int bytes_read = m_remoteFile->read(buffer, cnt);
    if (bytes_read <= 0) {
        HJFLoge("error, failed to read from remote file, pos: {}, cnt: {}, ret:{}, {}", file_pos, cnt, bytes_read, HJ_AVErr2Str(bytes_read));
        return bytes_read < 0 ? HJErrNetUnkown : 0;
    }

    bool block_complete = false;
    size_t written = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_blocks.find(block_idx);
        if (it == m_blocks.end() || it->second != block) {
            return bytes_read;
        }
        written = block->write(offset, static_cast<const uint8_t*>(buffer), static_cast<size_t>(bytes_read));
        block_complete = block->isComplete();
    }
    if (written != static_cast<size_t>(bytes_read)) {
        HJFLogw("warning, block write mismatch, expected: {}, actual: {}", bytes_read, written);
    }

    // 检查块是否完整，如果完整则写入本地文件
    if (block_complete) {
        int write_res = writeBlockToFile(block_idx);
        if (write_res != HJ_OK) {
            HJFLogw("warning, failed to write block {} to file, will retry later", block_idx);
            // 写入失败不影响读取，块保留在缓存中待重试
        }
    }

    return bytes_read;
}

int HJBlobSource::readFromBlockCache(void* buffer, size_t block_idx, size_t offset, size_t cnt)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_blocks.find(block_idx);
    if (it == m_blocks.end() || !it->second) {
        HJFLoge("error, block {} not found in cache", block_idx);
        return HJErrNotFind;
    }

    auto& block = it->second;
    const uint8_t* data = block->data();
    if (!data) {
        return HJErrInvalidData;
    }

    // 检查偏移和长度是否有效
    size_t valid_size = block->getValidSize();
    if (offset >= valid_size) {
        return 0;
    }

    size_t available = valid_size - offset;
    size_t to_copy = (std::min)(cnt, available);
    
    uint8_t* out = static_cast<uint8_t*>(buffer);
    std::copy(data + offset, data + offset + to_copy, out);
    return static_cast<int>(to_copy);
}

int HJBlobSource::writeBlockToFile(size_t block_idx)
{
    if (!m_writeFile) {
        HJFLoge("error, write file is null");
        return HJErrNotAlready;
    }

    HJBlockData::Ptr block;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_blocks.find(block_idx);
        if (it == m_blocks.end() || !it->second) {
            HJFLoge("error, block {} not found for writing", block_idx);
            return HJErrNotFind;
        }
        block = it->second;
        
        // 标记块正在写入
        m_writing_blocks.insert(block_idx);
    }

    int64_t write_pos = static_cast<int64_t>(block_idx) * m_db_info.block_size;
    if(write_pos != m_writeFile->tell()) 
    {
        HJFLogw("warning, write file pos mismatch, expected: {}, actual: {}", write_pos, m_writeFile->tell());
        int seek_res = m_writeFile->seek(write_pos, std::ios::beg);
        if (seek_res != 0) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_writing_blocks.erase(block_idx);
            HJFLoge("error, failed to seek write file to pos: {}", write_pos);
            return HJErrIO;
        }
    }

    // 写入块数据
    size_t valid_size = block->getValidSize();
    int write_res = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const uint8_t* data = block->data();
        if (!data) {
            m_writing_blocks.erase(block_idx);
            return HJErrInvalidData;
        }
        write_res = m_writeFile->write(data, valid_size);
    }
    if (write_res < 0 || static_cast<size_t>(write_res) != valid_size) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_writing_blocks.erase(block_idx);
        HJFLoge("error, failed to write block {} to file, expected: {}, actual: {}", 
                block_idx, valid_size, write_res);
        return HJErrIO;
    }

    // 刷新文件确保数据写入磁盘
    m_writeFile->flush();

    // 更新状态
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 更新块状态
        m_db_info.setBlockStatusAndUpdateSize(static_cast<int>(block_idx), true);
        
        // 从缓存中移除块
        m_blocks.erase(block_idx);
        m_writing_blocks.erase(block_idx);
        
        // 增加脏块计数
        m_dirty_block_count++;
    }

    // 检查是否需要持久化
    if (m_dirty_block_count >= kPersistThreshold) {
        saveFileInfo();
        m_dirty_block_count = 0;
    }

    // 检查文件是否完成
    if (m_db_info.isComplete()) {
        m_db_info.status = static_cast<int>(HJFileStatus::COMPLETED);
        saveFileInfo();
        HJFLogi("file download completed, rid: {}", m_local_rid);
    }

    return HJ_OK;
}

bool HJBlobSource::isBlockWriting(size_t block_idx) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_writing_blocks.find(block_idx) != m_writing_blocks.end();
}

int HJBlobSource::saveFileInfo()
{
    auto fileManager = HJLockWtr(m_fileManager);
    if (!fileManager) {
        HJFLoge("error, file manager is null");
        return HJErrNotAlready;
    }

    int res = fileManager->updateFileInfo(m_db_info);
    if (res != HJ_OK) {
        HJFLogw("warning, failed to save file info, rid: {}", m_local_rid);
    }
    return res;
}

int HJBlobSource::completeFileInfo()
{
    int res = HJ_OK;
    auto fileManager = HJLockWtr(m_fileManager);
    if (!fileManager) {
        HJFLoge("error, file manager is null");
        return HJErrNotAlready;
    }

    //int res = fileManager->updateFileInfo(m_db_info);
    //if (res != HJ_OK) {
    //    HJFLogw("warning, failed to save file info, rid: {}", m_local_rid);
    //}
    //m_writeFile->flush();
    //m_writeFile->close();
    //m_writeFile.reset();

    if(!m_info.local_url.empty() && !m_info.tmp_url.empty() && m_info.tmp_url != m_info.local_url) {
        res = fileManager->moveFile(m_db_info.category, m_info.tmp_rid,  m_info.tmp_url, m_info.rid, m_info.local_url);
    }
    m_fileManager.reset();

    return res;
}

int HJBlobSource::seek(int64_t offset, int whence)
{
    int64_t new_pos = m_pos;
    
    switch (whence) {
        case std::ios::beg: 
            new_pos = offset; 
            break;
        case std::ios::cur: 
            new_pos = m_pos + offset; 
            break;
        case std::ios::end: 
            new_pos = m_db_info.total_length + offset; 
            break;
        default: 
            return -1;
    }
    
    // 验证位置有效性
    if (new_pos < 0) {
        return -1;
    }
    
    // 只更新逻辑位置，不操作底层文件
    m_pos = new_pos;

    return 0;
}

int HJBlobSource::close()
{
    // 保存最终状态
    if (m_dirty_block_count > 0) {
        saveFileInfo();
        m_dirty_block_count = 0;
    }

    // 关闭远程文件
    if (m_remoteFile) {
        m_remoteFile->setQuit();
        m_remoteFile->close();
        m_remoteFile.reset();
    }

    // 关闭写入文件
    if (m_writeFile) {
        //
        m_writeFile->flush();
        m_writeFile->close();
        m_writeFile.reset();
    }

    // 关闭读取文件
    if (m_readFile) {
        m_readFile->close();
        m_readFile.reset();
    }
    completeFileInfo();

    // 清理块缓存
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_blocks.clear();
        m_writing_blocks.clear();
    }

    // 重置位置
    m_pos = 0;

    return HJ_OK;
}

void HJBlobSource::pruneBlocksLocked(size_t keep_idx)
{
    if (m_blocks.size() <= kMaxCachedBlocks) {
        return;
    }

    for (auto it = m_blocks.begin(); it != m_blocks.end() && m_blocks.size() > kMaxCachedBlocks;) {
        size_t candidate = it->first;
        if (candidate == keep_idx || m_writing_blocks.find(candidate) != m_writing_blocks.end()) {
            ++it;
            continue;
        }
        it = m_blocks.erase(it);
    }
}

HJBlockData::Ptr HJBlobSource::getOrCreateBlock(size_t global_offset)
{
    size_t block_idx = getBlockIndex(global_offset);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
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

    m_blocks[block_idx] = block;
    pruneBlocksLocked(block_idx);
    return block;
}

std::vector<size_t> HJBlobSource::getIncompleteBlocks() const
{
    std::vector<size_t> incomplete_blocks;
    int64_t total_blocks = m_db_info.blockCount();
    
    for (int64_t i = 0; i < total_blocks; ++i) {
        if (!m_db_info.getBlockStatus(static_cast<int>(i))) {
            incomplete_blocks.push_back(static_cast<size_t>(i));
        }
    }
    
    return incomplete_blocks;
}

NS_HJ_END
