//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJDataSource.h"
#include "HJFLog.h"
#include "HJLocalFileManager.h"
#include "HJDataSourceKit.h"
#include "HJFileUtil.h"
#include "HJException.h"
#include "HJFFUtils.h"
#include <algorithm>
#include <cmath>

NS_HJ_BEGIN

//***********************************************************************************//
// 构造与析构
//***********************************************************************************//
HJDataSource::HJDataSource(HJUrl::Ptr url, HJListener listener)
    : m_listener(listener) 
{
    auto fileManager = HJDataSourceKit::getInstance()->getFileManager();
    if (!fileManager) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, "file manager is null");
    }
    m_fileManager = fileManager;
    if (url) {
        open(url);
    }
}

HJDataSource::~HJDataSource() {
    close();
}

//***********************************************************************************//
// open() 实现
//***********************************************************************************//
int HJDataSource::open(HJUrl::Ptr url)
{
    if (!url) {
        return HJErrInvalidParams;
    }
    int res = HJXIOBase::open(url);
    if (HJ_OK != res) {
        return res;
    }

    do {
        m_remote_url = url->getUrl();
        m_local_dir = url->getString("local_dir");
        if(!HJFileUtil::isDirExist(m_local_dir)) {
            HJFLoge("error, local_dir: {} is not exist, try to create it", m_local_dir);
            return HJErrInvalidParams;
        }
        m_rid = url->getString("rid");
        if (m_remote_url.empty()) {
            HJFLoge("error, remote_url is empty");
            res = HJErrInvalidParams;
            break;
        }
        HJFLogi("entry, url: {}, rid: {}, local_url: {}", m_remote_url, m_rid, m_local_dir);

        auto blob = getSharedBlob(m_remote_url, m_local_dir, m_rid);
        if(!blob) {
            res = HJErrInvalidParams;
            HJFLoge("error, failed to get share blob, url: {}, rid: {}, local_dir: {}", m_remote_url, m_rid, m_local_dir);
            break;
        }
        m_blob = blob;
        //
        m_missingInfo = blob->getMissingInfo();
        m_total_length = m_missingInfo.totalLength;
        m_block_size = m_missingInfo.blockSize;

        HJFLogi("entry, total_length: {}, block_size: {}, in_complete_blocks: {}, hole_ranges: {}", m_total_length, m_block_size, HJMediaUtils::formatVector(m_missingInfo.incompleteBlocks), HJMediaUtils::formatVector(m_missingInfo.holeRanges));
    } while (false);

    if (res != HJ_OK) {
        closeRemoteFile();
        closeWriteFile();
        clearReadFileCache();
    }

    return res;
}

void HJDataSource::close()
{
    closeRemoteFile();
    closeWriteFile();
    clearReadFileCache();

    {
        // std::lock_guard<std::mutex> lock(m_mutex);
        m_downloading_blocks.clear();
        m_writing_blocks.clear();
    }
    if(m_blob) {
        m_blob->unlockWriteUrl(m_write_url);
    }
    m_blob.reset();

    m_pos = 0;
    m_total_length = 0;
    m_block_size = 0;

    m_remote_url.clear();
    m_write_url.clear();
    m_local_dir.clear();
    m_rid.clear();
    // m_category.clear();
}

HJShareBlob::Ptr HJDataSource::getSharedBlob(const std::string& remote_url, const std::string& local_dir, const std::string& rid)
{
    int res = HJ_OK;
    HJShareBlob::Ptr blob{};
    do
    {
        auto fileManager = HJLockWtr(m_fileManager);
        if (!fileManager) {
            HJFLoge("error, file manager is null");
            break;
        }
        blob = fileManager->getAreadyShareBlob(m_remote_url, m_local_dir, m_rid);
        if (!blob || !blob->isComplete()) 
        {
            auto murl = HJCreates<HJUrl>(m_remote_url, HJ_XIO_READ);
            if (m_url->haveValue("timeout")) {
                (*murl)["timeout"] = m_url->getInt64("timeout");
            }
            m_remoteFile = HJCreateu<HJXIOContext>();
            if (m_url->haveValue("interrupt_callback")) {
                AVIOInterruptCB interrupt_callback = m_url->getValue<AVIOInterruptCB>("interrupt_callback");
                m_remoteFile->setInterruptCB(&interrupt_callback);
            }
            res = m_remoteFile->open(murl);
            if (HJ_OK != res) {
                HJFLoge("error, failed to open remote url: {}, res:{}", m_remote_url, res);
                break;
            }

            auto file_total_length = m_remoteFile->size();
            if (file_total_length <= 0) {
                HJFLoge("error, remote file size is invalid");
                res = HJErrInvalidData;
                break;
            }
            m_total_length = file_total_length;

            HJFLogi("open remote file:{}", m_remote_url);
        }
        if (!blob && m_total_length > 0) {
            blob = fileManager->getShareBlob(m_remote_url, m_local_dir, m_rid, m_total_length);
        }
    } while (false);

    if (HJ_OK != res){
        return nullptr;
    }
    
    return blob;
}

//***********************************************************************************//
// 块索引计算
//***********************************************************************************//
size_t HJDataSource::getBlockIndex(size_t global_offset) const
{
    if (m_block_size <= 0) {
        return 0;
    }
    return global_offset / m_block_size;
}

size_t HJDataSource::getBlockOffset(size_t global_offset) const
{
    if (m_block_size <= 0) {
        return 0;
    }
    return global_offset % m_block_size;
}

//***********************************************************************************//
// 文件句柄管理
//***********************************************************************************//
HJXIOFile* HJDataSource::getOrOpenReadFile(const std::string& local_url)
{
    if (local_url.empty()) {
        return nullptr;
    }

    // 检查缓存中是否存在
    auto it = m_readFiles.find(local_url);
    if (it != m_readFiles.end() && it->second) {
        m_currentReadPath = local_url;
        return it->second.get();
    }

    // 超过最大缓存数时，清理最旧的（简单策略：清理第一个）
    if (m_readFiles.size() >= kMaxReadFileCache) {
        auto oldest = m_readFiles.begin();
        if (oldest != m_readFiles.end()) {
            if (oldest->second) {
                oldest->second->close();
            }
            m_readFiles.erase(oldest);
        }
    }

    // 打开新文件
    auto read_murl = HJCreates<HJUrl>(local_url, HJ_XIO_READ);
    auto readFile = HJCreateu<HJXIOFile>();
    int res = readFile->open(read_murl);
    if (res != HJ_OK) {
        HJFLoge("error, failed to open local url for read: {}", local_url);
        return nullptr;
    }

    HJXIOFile* ptr = readFile.get();
    m_readFiles[local_url] = std::move(readFile);
    m_currentReadPath = local_url;

    return ptr;
}

void HJDataSource::clearReadFileCache()
{
    for (auto& pair : m_readFiles) {
        if (pair.second) {
            pair.second->close();
        }
    }
    m_readFiles.clear();
    m_currentReadPath.clear();
}

HJXIOFile* HJDataSource::getWriteFile()
{
    if(m_writeFile) {
        return m_writeFile.get();
    }   
    if(m_write_url.empty()) {
        if(m_blob) {
            m_write_url = m_blob->lockWriteUrl();
        }
    }
    if(m_write_url.empty()) {
        HJFLoge("error, failed to lock local url, url: {}, rid: {}, local_dir: {}", m_remote_url, m_rid, m_local_dir);
        return nullptr;
    }

    HJXIOMode fmode = HJ_XIO_RWONLY;
    if (!HJFileUtil::isFileExist(m_write_url)) {
        fmode = HJ_XIO_WRITE;
    }
    auto write_murl = HJCreates<HJUrl>(m_write_url, fmode);
    m_writeFile = HJCreateu<HJXIOFile>();
    int res = m_writeFile->open(write_murl);
    if (HJ_OK != res) {
        HJFLoge("error, failed to open local url for write: {}, res: {}", m_write_url, res);
        return nullptr;
    }
    return m_writeFile.get();
}

void HJDataSource::closeWriteFile()
{
    if(!m_writeFile) {
        return;
    }
    m_writeFile->flush();
    m_writeFile->close();
    m_writeFile.reset();
    return;
}

void HJDataSource::closeRemoteFile()
{
    if(!m_remoteFile) {
        return;
    }
    m_remoteFile->setQuit();
    m_remoteFile->close();
    m_remoteFile.reset();

    return;
}

//***********************************************************************************//
// 读取策略实现
//***********************************************************************************//
int HJDataSource::readFromCached(void* buffer, HJBlockData::Ptr block, size_t offset, size_t cnt)
{
    if (!block) {
        return HJErrInvalidParams;
    }

    // 从 block 获取本地文件路径
    std::string local_url = block->getLocalUrl();
    if (local_url.empty()) {
        HJFLoge("error, cached block has no local_url, block_idx: {}", block->getID());
        return HJErrInvalidData;
    }

    // 获取或打开读取文件
    HJXIOFile* readFile = getOrOpenReadFile(local_url);
    if (!readFile) {
        HJFLoge("error, failed to get read file for: {}", local_url);
        return HJErrIO;
    }

    // 计算文件位置（块在文件中的位置 + 块内偏移）
    // 注意：对于 span 文件，需要计算块在该文件中的相对位置
    int64_t file_pos = static_cast<int64_t>(block->getGlobalStartOffset()) + offset;

    // 检查位置对齐
    if (file_pos != readFile->tell()) {
        int seek_res = readFile->seek(file_pos, std::ios::beg);
        if (seek_res != 0) {
            HJFLoge("error, failed to seek read file to pos: {}", file_pos);
            return HJErrIO;
        }
    }

    // 读取数据
    int bytes_read = readFile->read(buffer, cnt);
    if (bytes_read < 0) {
        HJFLoge("error, failed to read from local file, pos: {}, cnt: {}", file_pos, cnt);
        return HJErrIO;
    }

    return bytes_read;
}

int HJDataSource::readFromLocked(void* buffer, HJBlockData::Ptr block, size_t offset, size_t cnt)
{
    if (!block || !m_remoteFile) {
        HJFLoge("error, invalid remote file");
        return HJErrNotAlready;
    }
    int bytes_read = HJErrNetRecv;
    size_t block_idx = block->getID();
    m_downloading_blocks[block_idx] = block;
    do
    {
        int64_t remote_pos = static_cast<int64_t>(block->getGlobalStartOffset()) + offset;
        if (remote_pos != m_remoteFile->tell()) {
            int seek_res = m_remoteFile->seek(remote_pos, std::ios::beg);
            if (seek_res != HJ_OK) {
                HJFLoge("error, failed to seek remote file to pos: {}, res: {}", 
                        remote_pos, HJ_AVErr2Str(seek_res));
                break;
            }
        }
        bytes_read = m_remoteFile->read(buffer, cnt);
        if (bytes_read <= 0) {
            HJFLoge("error, failed to read from remote file, pos: {}, cnt: {}, ret: {}", 
                    remote_pos, cnt, HJ_AVErr2Str(bytes_read));
            break;
        }

        size_t written = block->write(offset, static_cast<const uint8_t*>(buffer), 
                                        static_cast<size_t>(bytes_read));
        if (written != static_cast<size_t>(bytes_read)) {
            HJFLogw("warning, block write mismatch, expected: {}, actual: {}", 
                    bytes_read, written);
        }
        auto block_complete = block->isComplete();
        if (block_complete) {
            int write_res = writeBlockToFile(block_idx, block);
            if (write_res != HJ_OK) {
                HJFLogw("warning, failed to write block {} to file, will retry later", block_idx);
                // 写入失败不影响读取，块保留在缓存中待重试
                return write_res;
            }
        }
    } while (false);

    if (m_blob) {
        m_blob->releaseBlock(block_idx);
    }

    return bytes_read;
}

int HJDataSource::readFromAlone(void* buffer, HJBlockData::Ptr block, size_t offset, size_t cnt)
{
    if (!block || !m_remoteFile) {
        return HJErrNotAlready;
    }

    // 计算远程文件位置
    int64_t remote_pos = static_cast<int64_t>(block->getGlobalStartOffset()) + offset;

    // 检查位置对齐
    if (remote_pos != m_remoteFile->tell()) {
        int seek_res = m_remoteFile->seek(remote_pos, std::ios::beg);
        if (seek_res != HJ_OK) {
            HJFLoge("error, failed to seek remote file to pos: {}, res: {}", 
                    remote_pos, HJ_AVErr2Str(seek_res));
            return HJErrNetUnkown;
        }
    }

    // 直接读取到输出缓冲区，不缓存
    int bytes_read = m_remoteFile->read(buffer, cnt);
    if (bytes_read <= 0) {
        HJFLoge("error, failed to read from remote file (ALONE), pos: {}, cnt: {}, ret: {}", 
                remote_pos, cnt, HJ_AVErr2Str(bytes_read));
        return bytes_read < 0 ? HJErrNetUnkown : 0;
    }

    // ALONE 块不写入 HJBlockData，不写入本地文件，不调用 commitBlock
    return bytes_read;
}

int HJDataSource::readFromBlockCache(void* buffer, size_t block_idx, size_t offset, size_t cnt)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_downloading_blocks.find(block_idx);
    if (it == m_downloading_blocks.end() || !it->second) {
        HJFLoge("error, block {} not found in download cache", block_idx);
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

//***********************************************************************************//
// 块写入
//***********************************************************************************//
int HJDataSource::writeBlockToFile(size_t block_idx, HJBlockData::Ptr block)
{
    if (!block) {
        HJFLoge("error, block is null");
        return HJErrInvalidParams;
    }
    HJXIOFile* write_file = getWriteFile();
    if (!write_file) {
        HJFLoge("error, failed to get write file");
        return HJErrIO;
    }
    int res = HJ_OK;
    // m_writing_blocks.insert(block_idx);
    do
    {
        int64_t write_pos = static_cast<int64_t>(block->getGlobalStartOffset());
        if (write_pos != write_file->tell()) {
            res = write_file->seek(write_pos, std::ios::beg);
            if (res != HJ_OK) {
                HJFLoge("error, failed to seek write file to pos: {}, res: {}", write_pos, res);
                break;
            }
        }
        size_t valid_size = block->getValidSize();
        const uint8_t* data = block->data();
        if (!data) {
            res = HJErrInvalidData;
            HJFLoge("error, block {} has no data", block_idx);
            break;
        }
        int written = write_file->write(data, valid_size);
        if (written < HJ_OK || static_cast<size_t>(written) != valid_size) {
            HJFLoge("error, failed to write block {} to file, expected: {}, actual: {}", 
                    block_idx, valid_size, written);
            res = HJErrIO;
            break;
        }
        write_file->flush();

        if (m_blob) {
            m_blob->commitBlock(block_idx, m_write_url);
        }
        m_downloading_blocks.erase(block_idx);
    } while (false);
    // m_writing_blocks.erase(block_idx);

    if(HJ_OK != res) {
        block->clearBuffer();
    }

    HJFLogi("block {} written to file successfully", block_idx);
    return res;
}

bool HJDataSource::isBlockWriting(size_t block_idx) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_writing_blocks.find(block_idx) != m_writing_blocks.end();
}

int HJDataSource::read(void* buffer, size_t cnt)
{
    if (!buffer || cnt == 0) {
        return 0;
    }

    if (!m_blob) {
        HJFLoge("error, blob is null");
        return HJErrNotAlready;
    }

    size_t total_read = 0;
    uint8_t* ptr = static_cast<uint8_t*>(buffer);

    while (total_read < cnt) {
        // 检查是否到达文件末尾
        if (m_pos >= m_total_length) {
            HJFLogw("pos:{}, total length:{}, reached EOF", m_pos, m_total_length);
            break; // EOF
        }

        // 计算当前块信息
        size_t block_idx = getBlockIndex(static_cast<size_t>(m_pos));
        size_t block_offset = getBlockOffset(static_cast<size_t>(m_pos));

        // 计算本次可读取的字节数（不超过块边界）
        size_t bytes_remaining_in_block = m_block_size - block_offset;
        size_t bytes_to_read = (std::min)(cnt - total_read, bytes_remaining_in_block);

        // 确保不超过文件末尾
        if (m_pos + static_cast<int64_t>(bytes_to_read) > m_total_length) {
            bytes_to_read = static_cast<size_t>(m_total_length - m_pos);
        }

        int bytes_read = 0;
        auto block = m_blob->getBlock(block_idx);
        if (!block) {
            HJFLoge("error, failed to get block {}", block_idx);
            if (total_read == 0) {
                return HJErrNotFind;
            }
            break;
        }

        // 根据块状态选择读取策略
        HJBlockData::BlockStatus status = block->getStatus();
        switch (status) {
            case HJBlockData::BlockStatus::CACHED:
                bytes_read = readFromCached(ptr + total_read, block, block_offset, bytes_to_read);
                break;
            case HJBlockData::BlockStatus::LOCKED:
                bytes_read = readFromLocked(ptr + total_read, block, block_offset, bytes_to_read);
                break;
            case HJBlockData::BlockStatus::ALONE:
                bytes_read = readFromAlone(ptr + total_read, block, block_offset, bytes_to_read);
                break;
            default:
                HJFLoge("error, unknown block status: {}", static_cast<int>(status));
                bytes_read = HJErrInvalidData;
                break;
        }

        if (bytes_read <= HJ_OK) {
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

//***********************************************************************************//
// seek 实现
//***********************************************************************************//
int HJDataSource::seek(int64_t offset, int whence)
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
            new_pos = m_total_length + offset;
            break;
        default:
            return -1;
    }

    // 验证位置有效性
    if (new_pos < 0) {
        return HJErrIOSeek;
    }

    // 只更新逻辑位置，不操作底层文件
    m_pos = new_pos;

    return HJ_OK;
}

int HJDataSource::flush()
{
    if (m_writeFile) {
        return m_writeFile->flush();
    }
    return HJ_OK;
}

int64_t HJDataSource::tell()
{
    return m_pos;
}

int64_t HJDataSource::size()
{
    return m_total_length;
}

bool HJDataSource::eof()
{
    return m_pos >= m_total_length;
}

NS_HJ_END
