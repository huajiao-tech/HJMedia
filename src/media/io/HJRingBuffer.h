//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJFifo.h"

#define HJ_BUFFER_CAPACITY         (4 * 1024 * 1024)
#define HJ_READ_BACK_CAPACITY      (4 * 1024 * 1024)
#define HJ_SHORT_SEEK_THRESHOLD    (256 * 1024)

NS_HJ_BEGIN
//***********************************************************************************//
class HJRingBuffer
{
public:
	using Ptr = std::shared_ptr<HJRingBuffer>;
	HJRingBuffer(unsigned int capacity, int readBackCapacity);
	virtual ~HJRingBuffer();

	void reset() {
		if (m_fifo) {
			m_fifo->reset();
		}
		m_readPos = 0;
	}
	int size() {
		if (!m_fifo) {
			return 0;
		}
		return (int)(m_fifo->canReadSize() - m_readPos);
	}
	int space() {
		if (!m_fifo) {
			return 0;
		}
		return (int)m_fifo->canWriteSize();
	}
	int sizeOfReadBack() {
		return m_readPos;
	}

	int read(uint8_t* dest, int len);
	int write(const uint8_t* buf, int len);
	int writeFromCB(HJReadCB readCB, size_t len);
	int drain(int offset);
private:
	HJFifo::Ptr		m_fifo = nullptr;
	int					m_readBackCapacity = 0;
	int					m_readPos = 0;
};

NS_HJ_END