//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJXIOBase.h"
#include "HJNotify.h"
#include "HJShareBlob.h"
#include "HJXIOFile.h"
#include "HJXIOContext.h"
#include <mutex>
#include <set>

NS_HJ_BEGIN
//***********************************************************************************//
class HJLocalFileManager;

/**
 * HJDataSource - 共享数据源类
 * 
 * 支持多播放器共享的混合数据源，通过 HJShareBlob 实现块级别的共享管理：
 * - CACHED 块：从本地文件读取（可能来自不同的 span 文件）
 * - LOCKED 块：从远程读取并缓存到本地
 * - ALONE 块：从远程直接读取，不缓存（其他实例已锁定）
 * 
 * 设计模式：
 * - 策略模式：根据块状态选择读取策略
 * - 享元模式：多个 HJDataSource 共享同一 HJShareBlob
 */
class HJDataSource : public HJXIOBase
{
public:
    HJ_DECLARE_PUWTR(HJDataSource);

    explicit HJDataSource(HJUrl::Ptr url = nullptr, HJListener listener = nullptr);
    virtual ~HJDataSource();

    //==========================================================================
    // HJXIOBase 接口实现
    //==========================================================================
    virtual int open(HJUrl::Ptr url) override;
    virtual void close() override;
    virtual int read(void* buffer, size_t cnt) override;
    virtual int write(const void* buffer, size_t cnt) override {
        return HJErrNotSupport;
    }
    virtual int seek(int64_t offset, int whence = std::ios::cur) override;
    virtual int flush() override;
    virtual int64_t tell() override;
    virtual int64_t size() override;
    virtual bool eof() override;

    HJBlobMissingInfo& getMissingInfo() {
        return m_missingInfo;
    }
    int64_t getBlockSize() const { return m_block_size; }
protected:
    HJShareBlob::Ptr getSharedBlob(const std::string& remote_url, const std::string& local_dir, const std::string& rid);
    //==========================================================================
    // 块索引计算
    //==========================================================================
    
    /**
     * @brief 计算全局偏移对应的块索引
     */
    size_t getBlockIndex(size_t global_offset) const;
    
    /**
     * @brief 计算全局偏移在块内的偏移
     */
    size_t getBlockOffset(size_t global_offset) const;

    //==========================================================================
    // 读取策略（策略模式）
    //==========================================================================
    
    /**
     * @brief 从本地文件读取 CACHED 块
     * @param buffer 输出缓冲区
     * @param block 块数据对象（包含 local_url）
     * @param offset 块内偏移
     * @param cnt 期望读取的字节数
     * @return 实际读取的字节数，负数为错误码
     */
    int readFromCached(void* buffer, HJBlockData::Ptr block, size_t offset, size_t cnt);
    
    /**
     * @brief 从远程读取 LOCKED 块并缓存
     * @param buffer 输出缓冲区
     * @param block 块数据对象（用于缓存数据）
     * @param offset 块内偏移
     * @param cnt 期望读取的字节数
     * @return 实际读取的字节数，负数为错误码
     */
    int readFromLocked(void* buffer, HJBlockData::Ptr block, size_t offset, size_t cnt);
    
    /**
     * @brief 从远程直接读取 ALONE 块（不缓存）
     * @param buffer 输出缓冲区
     * @param block 块数据对象（仅用于位置计算）
     * @param offset 块内偏移
     * @param cnt 期望读取的字节数
     * @return 实际读取的字节数，负数为错误码
     */
    int readFromAlone(void* buffer, HJBlockData::Ptr block, size_t offset, size_t cnt);
    
    /**
     * @brief 从块缓存读取（块正在写入时）
     */
    int readFromBlockCache(void* buffer, size_t block_idx, size_t offset, size_t cnt);

    //==========================================================================
    // 块写入
    //==========================================================================
    
    /**
     * @brief 将完整块写入本地文件
     * @param block_idx 块索引
     * @param block 块数据对象
     * @return HJ_OK 成功，其他为错误码
     */
    int writeBlockToFile(size_t block_idx, HJBlockData::Ptr block);
    
    /**
     * @brief 检查块是否正在写入
     */
    bool isBlockWriting(size_t block_idx) const;

    //==========================================================================
    // 文件句柄管理
    //==========================================================================
    
    /**
     * @brief 获取或打开指定路径的读取文件句柄
     * @param local_url 本地文件路径
     * @return 文件句柄指针，失败返回 nullptr
     */
    HJXIOFile* getOrOpenReadFile(const std::string& local_url);
    
    /**
     * @brief 清理缓存的读取文件句柄
     */
    void clearReadFileCache();

    HJXIOFile* getWriteFile();
    void closeWriteFile();
    void closeRemoteFile();
private:
    //==========================================================================
    // 配置信息
    //==========================================================================
    HJListener                          m_listener{};
    std::string                         m_remote_url{};
    std::string                         m_write_url{};      // 写入文件路径
    std::string                         m_local_dir{};
    std::string                         m_rid{};

    //==========================================================================
    // 共享块管理器
    //==========================================================================
    HJShareBlob::Ptr                    m_blob;  // 强引用，控制 HJShareBlob 生命周期
    std::weak_ptr<HJLocalFileManager>   m_fileManager;

    //==========================================================================
    // 文件句柄
    //==========================================================================
    std::map<std::string, std::unique_ptr<HJXIOFile>> m_readFiles;  // 多个读取文件
    std::unique_ptr<HJXIOFile>          m_writeFile{};              // 单个写入文件
    std::unique_ptr<HJXIOContext>       m_remoteFile{};             // 远程文件
    std::string                         m_currentReadPath{};        // 当前读取文件路径
    static constexpr size_t             kMaxReadFileCache = 4;      // 最大缓存读取文件数

    //==========================================================================
    // 位置管理
    //==========================================================================
    int64_t                             m_pos{0};
    int64_t                             m_total_length{0};
    int                                 m_block_size{0};
    HJBlobMissingInfo                   m_missingInfo;

    //==========================================================================
    // 块缓存管理（正在下载的 LOCKED 块）
    //==========================================================================
    std::map<size_t, HJBlockData::Ptr>  m_downloading_blocks;
    std::set<size_t>                    m_writing_blocks;

    //==========================================================================
    // 线程安全
    //==========================================================================
    mutable std::mutex                  m_mutex;
};

NS_HJ_END
