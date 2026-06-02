//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"
#include "HJLocalIOUtils.h"
#include "HJMediaDBUtils.h"
#include "HJXIOFile.h"
#include "HJXIOContext.h"
#include "HJBlockData.h"
#include <mutex>
#include <set>

NS_HJ_BEGIN
//***********************************************************************************//
class HJLocalFileManager;

/**
 * HJBlobSource - 混合数据源类
 * 
 * 支持从本地或远程透明读取数据，实现本地缓存机制：
 * - 块存在于本地时从 m_readFile 读取
 * - 块不存在时从 m_remoteFile 读取并缓存到本地
 * - 使用 HJBlockData 作为块缓存，完整后写入 m_writeFile
 * - 读写分离，避免文件句柄冲突
 */
class HJBlobSource : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJBlobSource);

    explicit HJBlobSource(const HJXIOFileInfo& info, HJListener listener = nullptr);
    virtual ~HJBlobSource();

    //==========================================================================
    // 核心接口
    //==========================================================================
    
    /**
     * @brief 打开数据源
     * @return HJ_OK 成功，其他为错误码
     */
    int open();
    
    /**
     * @brief 读取数据
     * @param buffer 输出缓冲区
     * @param cnt 期望读取的字节数
     * @return 实际读取的字节数，负数为错误码
     */
    int read(void* buffer, size_t cnt);
    
    /**
     * @brief 定位读取位置
     * @param offset 偏移量
     * @param whence 定位模式：std::ios::beg, std::ios::cur, std::ios::end
     * @return 0 成功，-1 失败
     */
    int seek(int64_t offset, int whence = std::ios::cur);
    
    /**
     * @brief 关闭数据源
     * @return HJ_OK 成功
     */
    int close();

    //==========================================================================
    // 状态查询接口
    //==========================================================================
    
    /**
     * @brief 获取当前读取位置
     */
    int64_t tell() const { return m_pos; }
    
    /**
     * @brief 获取文件总大小
     */
    int64_t size() const { return m_db_info.total_length; }
    
    /**
     * @brief 是否到达文件末尾
     */
    bool eof() const { return m_pos >= m_db_info.total_length; }
    
    /**
     * @brief 文件是否已完全下载
     */
    bool isComplete() const { return m_db_info.isComplete(); }

    /**
     * @brief 获取所有未完成块的索引
     * @return 未完成块索引列表
     */
    std::vector<size_t> getIncompleteBlocks() const;

    
protected:
    //==========================================================================
    // 块索引计算
    //==========================================================================
    
    /**
     * @brief 计算全局偏移对应的块索引
     * @param global_offset 全局偏移
     * @return 块索引
     */
    size_t getBlockIndex(size_t global_offset) const {
        return global_offset / m_db_info.block_size;
    }
    
    /**
     * @brief 计算全局偏移在块内的偏移
     * @param global_offset 全局偏移
     * @return 块内偏移
     */
    size_t getBlockOffset(size_t global_offset) const {
        return global_offset % m_db_info.block_size;
    }
    
    /**
     * @brief 获取或创建块缓存
     * @param global_offset 全局偏移（用于定位块）
     * @return 块数据指针，失败返回 nullptr
     */
    HJBlockData::Ptr getOrCreateBlock(size_t global_offset);

    //==========================================================================
    // 读取策略
    //==========================================================================
    
    /**
     * @brief 从本地文件读取
     * @param buffer 输出缓冲区
     * @param block_idx 块索引
     * @param offset 块内偏移
     * @param cnt 期望读取的字节数
     * @return 实际读取的字节数，负数为错误码
     */
    int readFromLocal(void* buffer, size_t block_idx, size_t offset, size_t cnt);
    
    /**
     * @brief 从远程文件读取（并缓存到块）
     * @param buffer 输出缓冲区
     * @param block_idx 块索引
     * @param offset 块内偏移
     * @param cnt 期望读取的字节数
     * @return 实际读取的字节数，负数为错误码
     */
    int readFromRemote(void* buffer, size_t block_idx, size_t offset, size_t cnt);
    
    /**
     * @brief 从块缓存读取
     * @param buffer 输出缓冲区
     * @param block_idx 块索引
     * @param offset 块内偏移
     * @param cnt 期望读取的字节数
     * @return 实际读取的字节数，负数为错误码
     */
    int readFromBlockCache(void* buffer, size_t block_idx, size_t offset, size_t cnt);

    //==========================================================================
    // 块写入
    //==========================================================================
    
    /**
     * @brief 将完整块写入本地文件
     * @param block_idx 块索引
     * @return HJ_OK 成功，其他为错误码
     */
    int writeBlockToFile(size_t block_idx);
    
    /**
     * @brief 检查块是否正在写入
     * @param block_idx 块索引
     * @return true 正在写入
     */
    bool isBlockWriting(size_t block_idx) const;

    //==========================================================================
    // 状态持久化
    //==========================================================================
    
    /**
     * @brief 保存文件信息到数据库
     * @return HJ_OK 成功，其他为错误码
     */
    int saveFileInfo();
    int completeFileInfo();

private:
    void pruneBlocksLocked(size_t keep_idx);

    //==========================================================================
    // 配置信息
    //==========================================================================
    HJXIOFileInfo                       m_info;
    HJListener                          m_listener;
    HJDBFileInfo                        m_db_info;
    std::weak_ptr<HJLocalFileManager>   m_fileManager;

    //==========================================================================
    // 本地路径
    //==========================================================================
    std::string                         m_local_url{};
    std::string                         m_local_rid{};

    //==========================================================================
    // 文件句柄（读写分离）
    //==========================================================================
    std::unique_ptr<HJXIOFile>          m_readFile{};    // 本地读取
    std::unique_ptr<HJXIOFile>          m_writeFile{};   // 本地写入
    std::unique_ptr<HJXIOContext>       m_remoteFile{};  // 远程读取

    //==========================================================================
    // 位置管理
    //==========================================================================
    int64_t                             m_pos{0};

    //==========================================================================
    // 块缓存管理
    //==========================================================================
    std::map<size_t, HJBlockData::Ptr>  m_blocks;        // 正在下载的块缓存
    std::set<size_t>                    m_writing_blocks; // 正在写入的块集合
    static constexpr size_t             kMaxCachedBlocks = 128;

    //==========================================================================
    // 线程安全
    //==========================================================================
    mutable std::mutex                  m_mutex;

    //==========================================================================
    // 持久化控制
    //==========================================================================
    size_t                              m_dirty_block_count{0};
    static constexpr size_t             kPersistThreshold = 10; // 每10个块持久化一次
};

NS_HJ_END
