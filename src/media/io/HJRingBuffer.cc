//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRingBuffer.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJRingBuffer::HJRingBuffer(unsigned int capacity, int readBackCapacity)
{
	m_fifo = std::make_shared<HJFifo>(capacity + readBackCapacity, 1, 0);
	if (!m_fifo) {
		return;
	}
	m_readBackCapacity = readBackCapacity;
	HJLogi("buffer len:" + HJ2STR(m_fifo->canWriteSize()));
}

HJRingBuffer::~HJRingBuffer()
{
	m_fifo = nullptr;
}

int HJRingBuffer::read(uint8_t* dest, int len) {
	if (!m_fifo || len > size()) {
		return 0;
	}
	int ret = 0;
	//HJLogi("read len:" + HJ2STR(len) + ", read pos:" + HJ2STR(m_readPos));
	if (len > size()) {
		HJFLoge("error, read len:{} beyond size£º{}", len, size());
		return 0;
	}
	if (dest)
		ret = m_fifo->peak(dest, len, m_readPos);
	m_readPos += len;

	if (m_readPos > m_readBackCapacity) {
		m_fifo->drain((size_t)m_readPos - m_readBackCapacity);
		m_readPos = m_readBackCapacity;
	}

	return ret;
}

int HJRingBuffer::write(const uint8_t* buf, int len)
{
	if (!m_fifo || len > space()) {
		HJFLoge("error, write len:{} beyond space:{}", len, space());
		return HJErrENOSPC;
	}
	//HJLogi("write len:" + HJ2STR(len));
	int ret = m_fifo->write(buf, len);
    if(ret < 0) {
        return ret;
    }
    return len;
}

int HJRingBuffer::writeFromCB(HJReadCB readCB, size_t len)
{
	if (!m_fifo || len > space()) {
		HJFLoge("error, write len:{} beyond space:{}", len, space());
		return HJErrENOSPC;
	}
	//HJLogi("write len:" + HJ2STR(len));
    int ret = m_fifo->writeFromCB(readCB, &len);
    if(ret < 0) {
        return ret;
    }
    return len;
}

int HJRingBuffer::drain(int offset)
{
	if (offset < -sizeOfReadBack()) {
		HJFLoge("error, drain offset:{}, -sizeOfReadBack:{}", offset, -sizeOfReadBack());
		return HJErrENOSPC;
	}
	if (offset > size()) {
		HJFLoge("error, drain offset:{} beyond size:{}", offset, size());
		return HJErrENOSPC;
	}
	m_readPos += offset;
	return HJ_OK;
}

NS_HJ_END
