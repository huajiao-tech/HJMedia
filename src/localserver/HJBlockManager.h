//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaUtils.h"
#include "HJMediaStorageUtils.h"
#include "HJXIOBlob.h"
#include "HJFileUtil.h"
#include <functional>
#include <map>
#include <optional>
#include <tuple>
#include <vector>

NS_HJ_BEGIN
namespace detail {
class BlockWarmupStrategyBase;
}
//***********************************************************************************//
class HJBlockManager : public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJBlockManager);

    HJBlockManager();
    virtual ~HJBlockManager() noexcept;

    int init(const HJUrl::Ptr& murl);
	void done();

    int write(HJBuffer::Ptr buffer, const HJRange64i range);
    std::pair<HJBuffer::Ptr, HJRange64i> read(const HJRange64i range);

    std::optional<std::tuple<size_t, size_t, size_t>> getFetchRange(size_t preCacheSize);

private:
    friend class detail::BlockWarmupStrategyBase;
    struct BlockState {
        size_t length{0};
        std::vector<std::pair<size_t, size_t>> missingRanges;
        bool dirty{false};
        bool persisted{false};
    };

    HJBlock::Ptr getBlock(const size_t pos);
    HJBlock::Ptr getBlockByIndex(int index);
    BlockState& ensureBlockState(int index);
    bool isRangeCached(const BlockState& state, size_t begin, size_t length) const;
    void consumeRange(BlockState& state, size_t begin, size_t length);
    void warmupBlocks(HJFileStatus status);
    void loadRangeFromFile(const HJRange64i& range, const std::function<bool(int)>& predicate);
    bool loadBlockFromFile(int index);
    bool canLoadBlockFromDisk(int index) const;
    int blockIndex(int64_t pos) const;
    size_t blockLength(int index) const;
    HJRange64i blockRange(int index) const;
    size_t alignDown(size_t value) const;
    size_t alignUp(size_t value) const;
    std::vector<HJRange64i> buildWarmupRanges() const;
    bool writeToLocalFile(int64_t offset, const uint8_t* data, size_t length);
    void updateBlockStatusOnCompletion(int index);
private:
    HJUrl::Ptr                      m_murl{};
    std::optional<HJDBFileInfo>     m_dbinfo;
    HJXIOFile::Utr                  m_localFile{};
    size_t							m_pos = 0;
	size_t							m_fileLength = 0;    //文件总大小
	std::map<int, HJBlock::Ptr>		m_blocks;   //缓存块，int为块索引, HJBlock::Ptr为块数据对象
	size_t							m_blockSize = HJ_XIO_BLOCK_SIZE;
    size_t                          m_totalBlocks = 0;
    bool                            m_metadataDirty = false;
    std::map<int, BlockState>       m_blockStates;
};

NS_HJ_END
