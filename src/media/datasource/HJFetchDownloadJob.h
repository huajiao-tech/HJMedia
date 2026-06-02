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
#include "HJNetFetch.h"
#include "HJBlockAggregator.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <set>

NS_HJ_BEGIN
//***********************************************************************************//
class HJLocalFileManager;
class HJXIOBlobFile;

/**
 * HJFetchDownloadJob - 支持断点续传的下载任务
 *
 * 核心特性：
 * - 从数据库加载/创建文件元信息(HJDBFileInfo)
 * - 基于位图(block_status)扫描缺失块
 * - 使用块聚合器将连续缺失块合并为大范围下载（提升效率）
 * - 按 Storage_Block (32KB) 粒度管理状态，按 Fetch_Block (1MB) 粒度下载
 * - 单范围最多重试10次，超限则中止整个下载
 * - 线程安全，支持取消操作
 */
class HJFetchDownloadJob : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJFetchDownloadJob);

    static constexpr int kMaxRetries = 10;

    /**
     * @brief 构造函数
     * @param info 下载任务信息
     * @param listener 进度/状态回调
     */
    explicit HJFetchDownloadJob(const HJXIOFileInfo& info, HJListener listener = nullptr);
    virtual ~HJFetchDownloadJob();

    /**
     * @brief 执行下载任务
     * @return HJ_OK 成功，其他错误码表示失败
     */
    int run();

    /**
     * @brief 取消下载任务
     */
    void done();

    /**
     * @brief 获取下载进度 (0-100)
     */
    int getProgress() const;

    /**
     * @brief 是否已完成
     */
    bool isComplete() const;

    /**
     * @brief 设置块聚合器（可选，默认使用 HJRangeAggregator）
     */
    void setAggregator(HJBlockAggregator::Ptr aggregator) { m_aggregator = aggregator; }

private:
    /**
     * @brief 扫描位图获取缺失块列表
     * @return HJ_OK 成功
     */
    int loadMissingBlocks();

    /**
     * @brief 下载聚合后的范围（带重试逻辑）
     * @param range 聚合后的字节范围
     * @param range_blocks 该范围包含的 Storage_Block 索引列表
     * @return HJ_OK 成功
     */
    int downloadAggregatedRange(const HJRange64i& range, const std::vector<int64_t>& range_blocks);

    /**
     * @brief 处理接收到的数据块，按 Storage_Block 粒度写入
     * @param data 接收到的数据
     * @param data_range 数据的字节范围
     * @return HJ_OK 成功
     */
    int processReceivedData(const HJBuffer::Ptr& data, const HJRange64i& data_range);

    /**
     * @brief 持久化指定范围内已完成的 Storage_Block 并更新数据库
     * @return HJ_OK 成功
     */
    int persistCompletedBlocks();

    /**
     * @brief 计算块的字节范围
     * @param block_index 块索引
     * @return {start, end} 字节范围(inclusive)
     */
    HJRange64i getBlockRange(int64_t block_index) const;

    /**
     * @brief 根据字节偏移计算 Storage_Block 索引
     */
    int64_t getBlockIndex(int64_t byte_offset) const;

    /**
     * @brief 通知进度（每个 Storage_Block 完成时调用）
     */
    void notifyProgress();

    /**
     * @brief 通知错误
     */
    void notifyError(int code);

    /**
     * @brief 通用通知
     */
    int notify(const HJNotification::Ptr& ntf) {
        if (m_listener) {
            return m_listener(ntf);
        }
        return HJ_OK;
    }

    /**
     * @brief 从缺失块列表中提取指定范围对应的块索引
     */
    std::vector<int64_t> extractBlocksForRange(const HJRange64i& range) const;

private:
    HJXIOFileInfo                   m_info;             // 下载任务信息
    HJDBFileInfo                    m_db_info;          // 数据库文件信息(含位图)
    HJListener                      m_listener;         // 进度/状态回调

    std::weak_ptr<HJLocalFileManager> m_fileManager;    // 文件管理器
    HJNetFetch::Ptr                 m_fetch;            // 网络下载器
    std::shared_ptr<HJXIOBlobFile>  m_blob;             // 分块文件写入器
    HJBlockAggregator::Ptr          m_aggregator;       // 块聚合器
    std::string                     m_hjtd_url;         // 临时文件路径

    std::vector<int64_t>            m_missing_blocks;   // 待下载块索引列表 (Storage_Block)
    std::set<int64_t>               m_completed_in_range; // 当前范围内已完成的块索引

    std::atomic<bool>               m_abort{false};     // 取消标志
    mutable std::mutex              m_fetch_mutex;      // fetch访问互斥
    int                             m_retry_count{0};   // 当前范围重试计数

    int64_t                         m_total_blocks{0};  // 总块数
    int64_t                         m_completed_blocks{0}; // 已完成块数(含之前已存在的)
};

NS_HJ_END
