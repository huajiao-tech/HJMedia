//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJBlockData.h"
#include "HJLocalIOUtils.h"
#include "HJMediaDBUtils.h"
#include "HJNotify.h"
#include "HJFileUtil.h"
#include <optional>

NS_HJ_BEGIN
//***********************************************************************************//
struct HJBlobMissingInfo
{
    std::vector<size_t>     incompleteBlocks;
    std::vector<HJRange64i> holeRanges;
    int64_t                 validSize{0};
    int64_t                 totalLength{0};
    int64_t                 blockSize{0};
    int                     status{static_cast<int>(HJFileStatus::NONE)};
};

/**
 * HJShareBlob - 多播放器共享块数据管理器
 * 
 * 核心功能：
 * - 从 HJDBFileInfo 构建块队列信息
 * - 管理块状态：HOLE/LOCKED/CACHED/ALONE
 * - 支持多播放器并发访问
 * - Span 合并与查询
 * 
 * 状态转换规则：
 * - CACHED: 直接返回，状态不变
 * - HOLE -> LOCKED: 获取时锁定
 * - LOCKED -> 返回 ALONE 副本: 已被锁定时复制返回
 */
class HJShareBlob : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJShareBlob);

    explicit HJShareBlob(const HJDBFileInfo& info, HJListener listener = nullptr);
    virtual ~HJShareBlob();

    struct HJSourceWriterUrl {
        std::string local_url;
        bool is_used{ false };          //是否占用

        HJSourceWriterUrl() = default;
        HJSourceWriterUrl(const std::string& url, bool used) : local_url(url), is_used(used) {}
    };
    //==========================================================================
    // 初始化接口
    //==========================================================================
    
    /**
     * @brief 初始化块队列
     * 根据 HJDBFileInfo 的 block_status 位图和 spans 信息初始化所有块
     * @return HJ_OK 成功
     */
    int init();
    
    /**
     * @brief 销毁资源
     */
    int done();

    //==========================================================================
    // 核心块获取接口
    //==========================================================================
    
    /**
     * @brief 获取指定索引的块
     * 
     * 状态转换规则：
     * - CACHED: 直接返回，状态不变
     * - HOLE: 转为 LOCKED，返回该块
     * - LOCKED: 复制一份，返回状态为 ALONE 的新块
     * 
     * @param block_idx 块索引
     * @return 块数据指针，失败返回 nullptr
     */
    HJBlockData::Ptr getBlock(size_t block_idx);

    //==========================================================================
    // 状态转换接口
    //==========================================================================
    
    /**
     * @brief 提交块下载成功
     * 将 LOCKED 状态转为 CACHED，并更新 span 信息
     * 
     * @param block_idx 块索引
     * @param local_url 块数据存储的本地文件路径
     * @return HJ_OK 成功
     */
    int commitBlock(size_t block_idx, const std::string& local_url);
    
    /**
     * @brief 释放块下载失败
     * 将 LOCKED 状态转回 HOLE
     * 
     * @param block_idx 块索引
     * @return HJ_OK 成功
     */
    int releaseBlock(size_t block_idx);

    //==========================================================================
    // 查询接口
    //==========================================================================
    /**
     * @brief 获取块状态
     */
    HJBlockData::BlockStatus getBlockStatus(size_t block_idx) const;
    
    /**
     * @brief 获取块对应的本地文件URL
     * 仅对 CACHED 状态有效
     */
    std::string getBlockLocalUrl(size_t block_idx) const;

    bool isComplete() const {
        HJ_AUTO_LOCK(m_mutex);
        if(m_db_info.status == static_cast<int>(HJFileStatus::COMPLETED)) {
            return true;
        } 
        return m_db_info.isComplete(); 
    }

    std::string lockWriteUrl();
    void unlockWriteUrl(const std::string& write_url);

    HJBlobMissingInfo getMissingInfo();


    std::vector<HJRange64i> getHoleRanges();

    // /**
    //  * @brief 获取数据库文件信息
    //  */
    // const HJDBFileInfo& dbInfo() const { return m_db_info; }
    int64_t totalLength() const { return m_db_info.total_length; }
    int blockSize() const { return m_db_info.block_size; }

    // int getStatus() const { return m_db_info.status; }

    std::vector<size_t> getIncompleteBlocks() const
    {
        std::vector<size_t> blocks;
        int64_t total_blocks = m_db_info.blockCount();
        
        for (int64_t i = 0; i < total_blocks; ++i) {
            if (!m_db_info.getBlockStatus(static_cast<int>(i))) {
                blocks.push_back(static_cast<size_t>(i));
            }
        }
        
        return blocks;
    }
    std::vector<size_t> getCompleteBlocks() const
    {
        std::vector<size_t> blocks;
        int64_t total_blocks = m_db_info.blockCount();

        for (int64_t i = 0; i < total_blocks; ++i) {
            if (m_db_info.getBlockStatus(static_cast<int>(i))) {
                blocks.push_back(static_cast<size_t>(i));
            }
        }

        return blocks;
    }
protected:
    /**
     * @brief 根据块索引获取或创建 HJBlockData
     */
    HJBlockData::Ptr getOrCreateBlock(size_t block_idx);
    
    /**
     * @brief 从 spans 中查找块对应的本地文件
     */
    std::string findLocalUrlInSpans(size_t block_idx) const;
    
    /**
     * @brief 合并 span 信息
     * 将连续的块合并到同一个 span 中
     */
    void mergeSpans(size_t block_idx, const std::string& local_url);

    int notify(const HJNotification::Ptr& ntf) {
        if (m_listener) {
            return m_listener(ntf);
        }
        return HJ_OK;
    }
    /**
     * @brief 获取块总数
     */
    size_t blockCount() const;

    /**
     * @brief 查询块对应的 span 信息
     * @param block_idx 块索引
     * @return span 信息（如果存在）
     */
    std::optional<HJDBSpanInfo> findSpanForBlock(size_t block_idx) const;
    
protected:
    mutable std::mutex                  m_mutex;
    HJListener                          m_listener;
    HJDBFileInfo                        m_db_info;
    std::map<size_t, HJBlockData::Ptr>  m_blocks;
    std::unordered_map<std::string, HJSourceWriterUrl> m_write_urls;

    size_t                              m_dirty_block_count{0};
    static constexpr size_t             kPersistThreshold = 10; // 每10个块持久化一次
};

NS_HJ_END