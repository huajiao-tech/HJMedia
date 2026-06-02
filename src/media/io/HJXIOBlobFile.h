//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJXIOBase.h"
#include "HJXIOFile.h"
#include "HJBlockData.h"
#include "HJNotify.h"
#include "HJFileUtil.h"
#include <map>
#include <vector>
#include <mutex>
#include <memory>

NS_HJ_BEGIN
//***********************************************************************************//

/**
 * HJXIOBlobFile - 带缓存的文件读写类
 * 
 * 核心特性：
 * - 基于 HJBlockData 的分块管理（固定32KB/块）
 * - 块级写入状态追踪（std::vector<bool>）
 * - 自动刷盘机制（块写满即落盘）
 * - 支持末尾非完整块处理
 * - 线程安全
 */
class HJXIOBlobFile : public HJXIOBase
{
public:
    HJ_DECLARE_PUWTR(HJXIOBlobFile);

    explicit HJXIOBlobFile(HJUrl::Ptr url = nullptr, HJListener listener = nullptr);
    virtual ~HJXIOBlobFile();

    virtual int open(HJUrl::Ptr url) override;
    virtual void close() override;
    
    /**
     * @brief 读取数据
     * 优先从内存缓存块读取，未命中则直接从物理文件读取
     */
    virtual int read(void* buffer, size_t cnt) override;
    
    /**
     * @brief 写入数据
     * 写入内存块 -> 检查完整性 -> 完整则自动刷盘并更新状态
     */
    virtual int write(const void* buffer, size_t cnt) override;
    
    virtual int seek(int64_t offset, int whence = std::ios::cur) override;
    virtual int flush() override;
    virtual int64_t tell() override;
    virtual int64_t size() override;
    virtual bool eof() override;

	virtual void setSize(const int64_t size) override;

    //=== 扩展接口 ===
    /**
     * @brief 检查整个文件是否已完整写入
     * 遍历所有块状态，确保均为 true
     */
    bool isComplete() const;
    
    /**
     * @brief 获取指定块的状态
     * @param index 块索引
     * @return true 表示该块已完整写入
     */
    bool isBlockComplete(size_t index) const;

	static size_t getBlockSize() { return HJXIOBlobFile::kBlockSize; }

    bool isClearupAfterWrited() const { return m_clearupAfterWrited; }
    void setClearupAfterWrited(bool clearup) { m_clearupAfterWrited = clearup; }
protected:
    /**
     * @brief 获取或创建指定全局偏移对应的块
     * @param global_offset 全局字节偏移
     * @return 块对象智能指针
     */
    HJBlockData::Ptr getOrCreateBlock(size_t global_offset);

    /**
     * @brief 将指定块数据写入物理文件
     * @param block 待写入的块
     * @return 成功返回 0，失败返回 -1
     */
    int flushBlock(const HJBlockData::Ptr& block);

    /**
     * @brief 确保状态追踪器容量足够
     * @param file_size 预期文件大小
     */
    void ensureBlockStatusCapacity(size_t file_size);

    /**
     * @brief 计算给定偏移对应的块索引
     */
    size_t getBlockIndex(size_t global_offset) const;

    /**
     * @brief 物理文件操作（无缓冲）
     * 用于刷盘或缓存未命中时的直接读取
     */
    int writeToFile(int64_t offset, const void* data, size_t len);
    int readFromFile(int64_t offset, void* data, size_t len);

	virtual int notify(HJNotification::Ptr ntf) {
        if (m_listener) {
            return m_listener(std::move(ntf));
        }
        return HJ_OK;
    }
	void addCompletedBlockCount(HJBlockData::Ptr block) {
		m_completed_block_count++;
		//
		if(m_completed_block_count >= m_block_status.size()) {
            auto ntfy = HJMakeNotification((size_t)HJFileStatus::COMPLETED, m_block_status.size());
            (*ntfy)["percent"] = 100;
            (*ntfy)["block"] = block;
            notify(std::move(ntfy));
		} else {
			int64_t percent = (m_completed_block_count * 100) / m_block_status.size();
            auto ntfy = HJMakeNotification((size_t)HJFileStatus::PENDING, block ? block->getID() : -1);
            (*ntfy)["percent"] = percent;
            (*ntfy)["block"] = block;
            notify(std::move(ntfy));  // 修复：添加缺失的 notify 调用
		}
	}
private:
	HJListener 						m_listener;
    std::unique_ptr<HJXIOFile>      m_file;             // 物理文件操作代理
    size_t                          m_pos{0};           // 当前逻辑读写指针
    
    // 活跃块管理：<BlockIndex, BlockPtr>
    // 使用 map 稀疏存储活跃的内存块
    std::map<size_t, HJBlockData::Ptr> m_blocks;
    bool                            m_clearupAfterWrited{true};
    
    // 块级写入状态追踪：Index -> IsComplete
    // true 表示该块已完整写入且已落盘（或无需再次落盘）
    std::vector<bool>               m_block_status;
    size_t                          m_completed_block_count{0}; // 已完成块计数

    
    mutable std::mutex              m_mutex;            // 线程安全互斥锁
    
    static constexpr size_t kBlockSize = HJBlockData::kBlockSize;
};

NS_HJ_END