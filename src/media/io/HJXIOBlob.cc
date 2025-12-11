//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXIOBlob.h"
#include "HJFLog.h"
#include <algorithm>
#include <cstring>

NS_HJ_BEGIN
//***********************************************************************************//
HJBlock::HJBlock(const int64_t start, const int64_t max)
	: m_start(start)
	, m_end(start)
	, m_max(max)
{

}

HJBlock::~HJBlock()
{
	m_buffer = nullptr;
}

size_t HJBlock::write(const uint8_t* buffer, size_t cnt)
{
	if (!m_buffer) {
		m_buffer = std::make_shared<HJBuffer>(m_max);
	}
	size_t wcnt = HJ_MIN(cnt, m_buffer->space());
	m_buffer->write(buffer, wcnt);
	//
	m_end = m_start + m_buffer->size();

	return wcnt;
}

HJBuffer::Ptr HJBlock::ensureBuffer(size_t capacity)
{
	if (!m_buffer) {
		const size_t allocSize = capacity > 0 ? capacity : static_cast<size_t>(m_max);
		m_buffer = std::make_shared<HJBuffer>(allocSize);
		m_buffer->setSize(0);
	}
	return m_buffer;
}

size_t HJBlock::writeAt(size_t offset, const uint8_t* buffer, size_t cnt)
{
	if (!buffer || cnt == 0 || offset >= static_cast<size_t>(m_max)) {
		return 0;
	}
	const size_t writable = std::min(cnt, static_cast<size_t>(m_max) - offset);
	auto buf = ensureBuffer(static_cast<size_t>(m_max));
	const size_t required = offset + writable;
	if (buf->size() < required) {
		buf->setSize(required);
	}
	std::memcpy(buf->data() + offset, buffer, writable);
	const int64_t newEnd = m_start + static_cast<int64_t>(required);
	if (newEnd > m_end) {
		m_end = newEnd;
	}
	return writable;
}

size_t HJBlock::readAt(size_t offset, uint8_t* buffer, size_t cnt) const
{
	if (!m_buffer || !buffer || cnt == 0) {
		return 0;
	}
	if (offset >= m_buffer->size()) {
		return 0;
	}
	const size_t readable = std::min(cnt, m_buffer->size() - offset);
	std::memcpy(buffer, m_buffer->data() + offset, readable);
	return readable;
}

//***********************************************************************************//
HJXIOBlob::HJXIOBlob(HJUrl::Ptr url)
{
	if (url) {
		open(url);
	}
}

HJXIOBlob::~HJXIOBlob()
{
	close();
}

int HJXIOBlob::open(HJUrl::Ptr url)
{
	if (!url || url->getUrl().empty()) {
		return HJErrInvalidParams;
	}
	int res = HJXIOBase::open(url);
	if (HJ_OK != res) {
		return res;
	}
	m_file = std::make_unique<HJXIOFile>();
	res = m_file->open(url);
	if (HJ_OK != res) {
		return res;
	}
	return res;
}

void HJXIOBlob::close()
{
	flush();
	m_blocks.clear();
	m_file = nullptr;
}

int HJXIOBlob::read(void* buffer, size_t cnt)
{
	int ret = 0;

	return ret;
}

int HJXIOBlob::write(const void* buffer, size_t cnt)
{
	size_t wtotal = 0;
	uint8_t* pdata = (uint8_t*)buffer;
	int64_t psize = (int64_t)cnt;
	while (psize > 0)
	{
		HJBlock::Ptr block = getBlock(m_pos);
		if (!block) {
			HJLoge("error, get block failed");
			return HJErrIOWrite;
		}
		//
		size_t wcnt = block->write(pdata, psize);
		pdata += wcnt;
		psize -= wcnt;
		wtotal += wcnt;
		//
		m_pos += wcnt;
		//
		if (m_file && block->isFull() && block->getStart() == m_file->tell()) {
			m_file->write(block->getBuffer()->data(), block->getBuffer()->size());
			block->setWrited(true);
		}
	}
	
	return (int)wtotal;
}

int HJXIOBlob::seek(int64_t offset, int whence)
{
	switch (whence)
	{
		case std::ios::beg:
			m_pos = offset;
			break;
		case std::ios::cur:
			m_pos += offset;
			break;
		case std::ios::end:
			if (m_size > 0) {
				m_pos = m_size + offset;
			}
			break;
		default: {
			HJFLogi("error, unsupport seek whence:{}", whence);
			return HJErrIOSeek;
		}
	}
	return HJ_OK;
}

int HJXIOBlob::flush()
{
	int res = HJ_OK;

	return res;
}

int64_t HJXIOBlob::tell()
{
	return m_pos;
}

int64_t HJXIOBlob::size()
{
	return m_size;
}

bool HJXIOBlob::eof()
{
	return false;
}

HJBlock::Ptr HJXIOBlob::getBlock(const size_t pos)
{
	HJBlock::Ptr block = nullptr;
	int idx = pos / m_blockSize;
	const auto& it = m_blocks.find(idx);
	if (it != m_blocks.end()) {
		block = it->second;
	}
	if (!block) {
		block = std::make_shared<HJBlock>(pos, m_blockSize);
		block->setID(idx);
		//
		m_blocks[idx] = block;
	}
	return block;
}

NS_HJ_END
