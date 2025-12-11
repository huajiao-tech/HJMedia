//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJBlockManager.h"
#include "HJFLog.h"
#include "HJException.h"
#include "HJXIOFile.h"
#include "HJFileUtil.h"
#include <algorithm>
#include <memory>

NS_HJ_BEGIN
namespace detail {
constexpr size_t kHeadWarmSize = 512 * 1024;
constexpr size_t kTailPercent = 20; // 5%

class BlockWarmupStrategyBase
{
public:
	virtual ~BlockWarmupStrategyBase() = default;
	virtual void warmup(HJBlockManager& manager) const = 0;
protected:
	void loadRanges(HJBlockManager& manager, const std::function<bool(int)>& predicate) const {
		const auto ranges = manager.buildWarmupRanges();
		for (const auto& range : ranges) {
			manager.loadRangeFromFile(range, predicate);
		}
	}
};

class CompletedWarmupStrategy final : public BlockWarmupStrategyBase
{
public:
	void warmup(HJBlockManager& manager) const override {
		loadRanges(manager, [](int) { return true; });
	}
};

class PendingWarmupStrategy final : public BlockWarmupStrategyBase
{
public:
	explicit PendingWarmupStrategy(const HJDBFileInfo& info)
		: m_info(info) {}

	void warmup(HJBlockManager& manager) const override {
		auto predicate = [this](int index) {
			return m_info.getBlockStatus(index);
		};
		loadRanges(manager, predicate);
	}
private:
	const HJDBFileInfo& m_info;
};

class NoopWarmupStrategy final : public BlockWarmupStrategyBase
{
public:
	void warmup(HJBlockManager&) const override {}
};

std::unique_ptr<BlockWarmupStrategyBase> makeWarmupStrategy(HJFileStatus status, const std::optional<HJDBFileInfo>& info)
{
	switch (status)
	{
	case HJFileStatus::COMPLETED:
		return std::make_unique<CompletedWarmupStrategy>();
	case HJFileStatus::PENDING:
		if (info.has_value()) {
			return std::make_unique<PendingWarmupStrategy>(*info);
		}
		return std::make_unique<NoopWarmupStrategy>();
	case HJFileStatus::NONE:
	default:
		return std::make_unique<NoopWarmupStrategy>();
	}
}
} // namespace detail
//***********************************************************************************//
HJBlockManager::HJBlockManager()
{

}
HJBlockManager::~HJBlockManager()
{
    done();
}

int HJBlockManager::init(const HJUrl::Ptr& murl)
{
	if (!murl) {
		return HJErrInvalidParams;
	}
	m_murl = murl;
	m_fileLength = std::max<int64_t>(0, m_murl->getInt64("file_length"));
	if (m_murl->haveValue("dbInfo")) {
		m_dbinfo = m_murl->getValue<HJDBFileInfo>("dbInfo");
	}
	if (m_dbinfo.has_value()) {
		if (m_dbinfo->total_length > 0) {
			m_fileLength = static_cast<size_t>(m_dbinfo->total_length);
		}
		if (m_dbinfo->block_size > 0) {
			m_blockSize = static_cast<size_t>(m_dbinfo->block_size);
		}
	}
	if (m_blockSize == 0) {
		m_blockSize = HJ_XIO_BLOCK_SIZE;
	}
	if (m_fileLength == 0) {
		HJFLoge("invalid file length for url:{}", m_murl->getUrl());
		return HJErrInvalid;
	}

	m_totalBlocks = (m_fileLength + m_blockSize - 1) / m_blockSize;

	m_localFile = HJCreateu<HJXIOFile>();
	int res = m_localFile->open(m_murl);
	if (HJ_OK != res) {
		HJFLoge("open local file failed, url:{}, res:{}", m_murl->getUrl(), res);
		return res;
	}

	HJFileStatus status = HJFileStatus::NONE;
	if (m_dbinfo.has_value()) {
		status = static_cast<HJFileStatus>(m_dbinfo->status);
	}
	warmupBlocks(status);
	return HJ_OK;
}

void HJBlockManager::done()
{
	if (m_localFile) {
		m_localFile->flush();
		m_localFile->close();
		m_localFile = nullptr;
	}
	//m_blocks.clear();
	//m_blockStates.clear();
	m_murl = nullptr;
	m_dbinfo.reset();
	m_totalBlocks = 0;
}

int HJBlockManager::write(HJBuffer::Ptr buffer, const HJRange64i range)
{
	if (!buffer || !m_localFile) {
		return HJErrInvalidParams;
	}
	// The manager treats ranges as [begin, end) to align with fetch notifications.
	if (range.end <= range.begin) {
		return 0;
	}
	const int64_t clampedEnd = std::min<int64_t>(range.end, static_cast<int64_t>(m_fileLength));
	if (clampedEnd <= range.begin) {
		return 0;
	}
	const size_t requested = static_cast<size_t>(clampedEnd - range.begin);
	const size_t payload = std::min(buffer->size(), requested);
	if (payload == 0) {
		return 0;
	}
	const uint8_t* pdata = buffer->data();
	size_t written = 0;
	int64_t cursor = range.begin;
	while (written < payload) {
		const int index = blockIndex(cursor);
		HJBlock::Ptr block = getBlockByIndex(index);
		if (!block) {
			HJFLoge("failed to locate block idx:{} for offset:{}", index, cursor);
			return HJErrIOWrite;
		}
		auto& state = ensureBlockState(index);
		const auto blkRange = blockRange(index);
		const int64_t blkStart = blkRange.begin;
		if (blkStart < 0) {
			break;
		}
		const size_t offset = static_cast<size_t>(cursor - blkStart);
		const size_t blockWritable = state.length > offset ? (state.length - offset) : 0;
		if (blockWritable == 0) {
			break;
		}
		const size_t chunk = std::min(blockWritable, payload - written);
		const size_t wcnt = block->writeAt(offset, pdata + written, chunk);
		if (wcnt != chunk) {
			HJFLoge("write block failed, idx:{}, expected:{}, real:{}", index, chunk, wcnt);
			return HJErrIOWrite;
		}
		if (!writeToLocalFile(blkStart + static_cast<int64_t>(offset), pdata + written, chunk)) {
			HJFLoge("write local file failed, idx:{}, offset:{}", index, blkStart + offset);
			return HJErrIOWrite;
		}
		consumeRange(state, offset, chunk);
		if (state.missingRanges.empty()) {
			updateBlockStatusOnCompletion(index);
		} else {
			block->setStatus(HJBlock::BlockStatus::PENDING);
			state.dirty = true;
		}
		written += chunk;
		cursor += static_cast<int64_t>(chunk);
	}
	return static_cast<int>(written);
}


std::pair<HJBuffer::Ptr, HJRange64i> HJBlockManager::read(const HJRange64i range)
{
	if (range.end <= range.begin || !m_localFile) {
		return {};
	}
	const int64_t clampedEnd = std::min<int64_t>(range.end, static_cast<int64_t>(m_fileLength));
	if (clampedEnd <= range.begin) {
		return {};
	}
	const size_t requestBytes = static_cast<size_t>(clampedEnd - range.begin);
	if (requestBytes == 0) {
		return {};
	}
	auto result = HJCreates<HJBuffer>(requestBytes);
	uint8_t* dst = result->data();
	size_t copied = 0;
	int64_t cursor = range.begin;
	while (cursor < clampedEnd) {
		const int index = blockIndex(cursor);
		HJBlock::Ptr block = getBlockByIndex(index);
		if (!block) {
			break;
		}
		auto& state = ensureBlockState(index);
		const auto blkRange = blockRange(index);
		const size_t offset = static_cast<size_t>(cursor - blkRange.begin);
		const size_t blockReadable = state.length > offset ? (state.length - offset) : 0;
		if (blockReadable == 0) {
			break;
		}
		const size_t chunk = std::min(blockReadable, static_cast<size_t>(clampedEnd - cursor));
		if (!isRangeCached(state, offset, chunk)) {
			if (!canLoadBlockFromDisk(index) || !loadBlockFromFile(index) || !isRangeCached(state, offset, chunk)) {
				break;
			}
			block = getBlockByIndex(index);
		}
		const size_t rcnt = block->readAt(offset, dst + copied, chunk);
		if (rcnt == 0) {
			break;
		}
		copied += rcnt;
		cursor += static_cast<int64_t>(rcnt);
		if (rcnt < chunk) {
			break;
		}
	}
	if (copied == 0) {
		return {};
	}
	result->setSize(copied);
	HJRange64i actual{range.begin, range.begin + static_cast<int64_t>(copied) - 1};
	return {result, actual};
}

std::optional<std::tuple<size_t, size_t, size_t>> HJBlockManager::getFetchRange(size_t preCacheSize)
{
	if (preCacheSize == 0 || m_fileLength == 0) {
		return {};
	}
	size_t startIndex = 0;
	if (m_dbinfo.has_value() && m_dbinfo->blockCount() > 0) {
		const size_t count = static_cast<size_t>(m_dbinfo->blockCount());
		bool found = false;
		for (size_t i = 0; i < count; ++i) {
			if (!m_dbinfo->getBlockStatus(static_cast<int>(i))) {
				startIndex = i;
				found = true;
				break;
			}
		}
		if (!found) {
			return {};
		}
	}
	const size_t start = startIndex * m_blockSize;
	if (start >= m_fileLength) {
		return {};
	}
	const size_t endExclusive = std::min(m_fileLength, start + preCacheSize);
	if (endExclusive <= start) {
		return {};
	}
	const size_t endInclusive = endExclusive - 1;
	return std::make_tuple(startIndex, start, endInclusive);
}

HJBlock::Ptr HJBlockManager::getBlock(const size_t pos)
{
	const int idx = static_cast<int>(pos / m_blockSize);
	const auto& it = m_blocks.find(idx);
	if (it != m_blocks.end()) {
		return it->second;
	}
	const auto range = blockRange(idx);
	if (range.end < range.begin) {
		return nullptr;
	}
	const size_t blkLength = static_cast<size_t>(range.end - range.begin + 1);
	auto block = std::make_shared<HJBlock>(range.begin, static_cast<int64_t>(blkLength));
	block->setID(idx);
	block->setMax(static_cast<int64_t>(blkLength));
	m_blocks[idx] = block;
	ensureBlockState(idx);
	return block;
}

HJBlock::Ptr HJBlockManager::getBlockByIndex(int index)
{
	if (index < 0) {
		return nullptr;
	}
	const size_t pos = static_cast<size_t>(index) * m_blockSize;
	return getBlock(pos);
}

HJBlockManager::BlockState& HJBlockManager::ensureBlockState(int index)
{
	const size_t length = blockLength(index);
	auto [it, inserted] = m_blockStates.try_emplace(index);
	if (inserted) {
		it->second.length = length;
		it->second.persisted = m_dbinfo.has_value() && m_dbinfo->getBlockStatus(index);
		if (length > 0) {
			it->second.missingRanges = { {0, length} };
		}
	}
	return it->second;
}

bool HJBlockManager::isRangeCached(const BlockState& state, size_t begin, size_t length) const
{
	if (length == 0) {
		return true;
	}
	if (state.missingRanges.empty()) {
		return true;
	}
	const size_t end = begin + length;
	for (const auto& hole : state.missingRanges) {
		if (end <= hole.first) {
			return true;
		}
		if (begin >= hole.second) {
			continue;
		}
		return false;
	}
	return true;
}

void HJBlockManager::consumeRange(BlockState& state, size_t begin, size_t length)
{
	if (length == 0 || state.missingRanges.empty()) {
		return;
	}
	const size_t end = begin + length;
	std::vector<std::pair<size_t, size_t>> updated;
	updated.reserve(state.missingRanges.size());
	for (const auto& hole : state.missingRanges) {
		if (end <= hole.first || begin >= hole.second) {
			updated.emplace_back(hole);
			continue;
		}
		if (begin > hole.first) {
			updated.emplace_back(hole.first, begin);
		}
		if (end < hole.second) {
			updated.emplace_back(end, hole.second);
		}
	}
	state.missingRanges.swap(updated);
	if (state.missingRanges.empty()) {
		state.persisted = true;
		state.dirty = false;
	}
}

void HJBlockManager::warmupBlocks(HJFileStatus status)
{
	auto strategy = detail::makeWarmupStrategy(status, m_dbinfo);
	if (strategy) {
		strategy->warmup(*this);
	}
}

void HJBlockManager::loadRangeFromFile(const HJRange64i& range, const std::function<bool(int)>& predicate)
{
	if (range.end < range.begin || !m_localFile) {
		return;
	}
	const int startIndex = blockIndex(range.begin);
	const int endIndex = blockIndex(range.end);
	for (int idx = startIndex; idx <= endIndex; ++idx) {
		if (predicate && !predicate(idx)) {
			continue;
		}
		loadBlockFromFile(idx);
	}
}

bool HJBlockManager::loadBlockFromFile(int index)
{
	if (index < 0 || !m_localFile) {
		return false;
	}
	const auto range = blockRange(index);
	if (range.end < range.begin) {
		return false;
	}
	const size_t length = static_cast<size_t>(range.end - range.begin + 1);
	if (length == 0) {
		return false;
	}
	HJBlock::Ptr block = getBlockByIndex(index);
	if (!block) {
		return false;
	}
	auto buffer = block->ensureBuffer(length);
	size_t total = 0;
	if (HJ_OK != m_localFile->seek(range.begin, std::ios::beg)) {
		HJFLoge("seek failed when loading block:{}, offset:{}", index, range.begin);
		return false;
	}
	while (total < length) {
		const size_t remain = length - total;
		int cnt = m_localFile->read(buffer->data() + total, remain);
		if (cnt <= 0) {
			break;
		}
		total += static_cast<size_t>(cnt);
	}
	if (total == 0) {
		return false;
	}
	buffer->setSize(total);
	auto& state = ensureBlockState(index);
	if (total >= length) {
		block->setStatus(HJBlock::BlockStatus::COMPLETED);
		state.missingRanges.clear();
		state.persisted = true;
	} else {
		block->setStatus(HJBlock::BlockStatus::PENDING);
		consumeRange(state, 0, total);
	}
	return true;
}

bool HJBlockManager::canLoadBlockFromDisk(int index) const
{
	if (!m_localFile) {
		return false;
	}
	if (!m_dbinfo.has_value()) {
		return true;
	}
	return m_dbinfo->getBlockStatus(index);
}

int HJBlockManager::blockIndex(int64_t pos) const
{
	if (pos <= 0) {
		return 0;
	}
	return static_cast<int>(pos / static_cast<int64_t>(m_blockSize));
}

size_t HJBlockManager::blockLength(int index) const
{
	if (index < 0) {
		return 0;
	}
	if (static_cast<size_t>(index) >= m_totalBlocks) {
		return 0;
	}
	const int64_t start = static_cast<int64_t>(index) * static_cast<int64_t>(m_blockSize);
	const int64_t end = std::min<int64_t>(static_cast<int64_t>(m_fileLength), start + static_cast<int64_t>(m_blockSize));
	return static_cast<size_t>(end - start);
}

HJRange64i HJBlockManager::blockRange(int index) const
{
	if (index < 0 || static_cast<size_t>(index) >= m_totalBlocks) {
		return {0, -1};
	}
	const int64_t start = static_cast<int64_t>(index) * static_cast<int64_t>(m_blockSize);
	const int64_t end = std::min<int64_t>(static_cast<int64_t>(m_fileLength) - 1,
		start + static_cast<int64_t>(m_blockSize) - 1);
	return {start, std::max(start, end)};
}

size_t HJBlockManager::alignDown(size_t value) const
{
	if (m_blockSize == 0) {
		return value;
	}
	return (value / m_blockSize) * m_blockSize;
}

size_t HJBlockManager::alignUp(size_t value) const
{
	if (m_blockSize == 0) {
		return value;
	}
	return ((value + m_blockSize - 1) / m_blockSize) * m_blockSize;
}

std::vector<HJRange64i> HJBlockManager::buildWarmupRanges() const
{
	std::vector<HJRange64i> ranges;
	if (m_fileLength == 0) {
		return ranges;
	}
	const size_t head = std::min(m_fileLength, alignUp(std::min(m_fileLength, detail::kHeadWarmSize)));
	if (head > 0) {
		ranges.push_back({0, static_cast<int64_t>(head - 1)});
	}
	if (head >= m_fileLength) {
		return ranges;
	}
	size_t tailBytes = m_fileLength / detail::kTailPercent;
	if (tailBytes < m_blockSize) {
		tailBytes = m_blockSize;
	}
	tailBytes = std::min(m_fileLength, alignUp(tailBytes));
	size_t tailStart = m_fileLength > tailBytes ? m_fileLength - tailBytes : 0;
	tailStart = alignDown(tailStart);
	if (!ranges.empty() && static_cast<size_t>(ranges.back().end) >= tailStart) {
		ranges.back().end = static_cast<int64_t>(m_fileLength - 1);
	} else {
		ranges.push_back({static_cast<int64_t>(tailStart), static_cast<int64_t>(m_fileLength - 1)});
	}
	return ranges;
}

bool HJBlockManager::writeToLocalFile(int64_t offset, const uint8_t* data, size_t length)
{
	if (!m_localFile || !data || length == 0) {
		return false;
	}
	if (HJ_OK != m_localFile->seek(offset, std::ios::beg)) {
		return false;
	}
	int cnt = m_localFile->write(data, length);
	return cnt == static_cast<int>(length);
}

void HJBlockManager::updateBlockStatusOnCompletion(int index)
{
	HJBlock::Ptr block = getBlockByIndex(index);
	if (block) {
		block->setStatus(HJBlock::BlockStatus::COMPLETED);
		block->setWrited(true);
	}
	if (m_dbinfo.has_value()) {
		m_dbinfo->setBlockStatusAndUpdateSize(index, true);
		m_metadataDirty = true;
	}
}

NS_HJ_END
