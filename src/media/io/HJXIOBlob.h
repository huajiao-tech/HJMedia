//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJXIOBase.h"
#include "HJXIOFile.h"

NS_HJ_BEGIN
#define HJ_XIO_BLOCK_SIZE		1 * 1024 * 1024
//***********************************************************************************//
class HJBlock : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJBlock>;
	HJBlock(const int64_t start = 0, const int64_t max = HJ_XIO_BLOCK_SIZE);
	virtual ~HJBlock();

	enum class BlockStatus {
		NONE,
		PENDING,
		COMPLETED,
		CORRUPTED
	};

	size_t write(const uint8_t* buffer, size_t cnt);

	bool isFull() const {
		return (m_end - m_start) >= m_max;
	}
	void setMax(const int64_t max) {
		m_max = max;
	}
	void setNext(const HJBlock::Ptr next) {
		m_next = next;
	}
	const HJBuffer::Ptr& getBuffer() const {
		return m_buffer;
	}
	const int64_t getStart() const {
		return m_start;
	}
	const int64_t getEnd() const {
		return m_end;
	}
	bool isHit(const int64_t pos) const {
		return (pos >= m_start && pos <= m_end);
	}
	bool isNext(const int64_t pos) const {
		return (pos > m_end);
	}
	bool isPre(const int64_t pos) const {
		return (pos < m_start);
	}
	void setWrited(const bool write) {
		m_writed = write;
	}
	const bool getWrited() const {
		return m_writed;
	}
    BlockStatus getStatus() const {
		return m_status;
	}
    void setStatus(const BlockStatus status) {
		m_status = status;
	}
protected:
	HJBlock::Ptr	m_next = nullptr;
	HJBuffer::Ptr	m_buffer = nullptr;
	int64_t			m_start = 0;
	int64_t			m_end = 0;
	int64_t			m_max = HJ_XIO_BLOCK_SIZE;
	bool			m_writed = false;
	BlockStatus		m_status = BlockStatus::NONE;
};

class HJXIOBlob : public HJXIOBase
{
public:
	using Ptr = std::shared_ptr<HJXIOBlob>;
	HJXIOBlob(HJUrl::Ptr url = nullptr);
	virtual ~HJXIOBlob();

	virtual int open(HJUrl::Ptr url) override;
	virtual void close() override;
	virtual int read(void* buffer, size_t cnt) override;
	virtual int write(const void* buffer, size_t cnt) override;
	virtual int seek(int64_t offset, int whence = std::ios::cur) override;
	virtual int flush() override;
	virtual int64_t tell() override;
	virtual int64_t size() override;
    virtual bool eof() override;
public:
	void setSize(const int64_t size) {
		m_size = size;
	}
	HJBlock::Ptr getBlock(const size_t pos);
protected:
	std::unique_ptr<HJXIOFile>		m_file = nullptr;
	size_t							m_pos = 0;
	size_t							m_size = 0;
	std::map<int, HJBlock::Ptr>		m_blocks;
	size_t							m_blockSize = HJ_XIO_BLOCK_SIZE;
};

NS_HJ_END
