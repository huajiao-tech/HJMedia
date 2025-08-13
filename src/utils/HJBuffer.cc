//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBuffer.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJBuffer::HJBuffer(const HJBuffer::Ptr& other)
{
    if (other->size() > 0) {
        write(other->data(), other->size());
    }
}
HJBuffer::HJBuffer(size_t capacity, size_t offset/* = 0*/)
    : m_offset(offset)
{
    setCapacity(capacity);
}
HJBuffer::HJBuffer(const uint8_t* data, size_t size)
{
    setData(data, size);
}
HJBuffer::~HJBuffer()
{
    if (m_data) {
        m_alloc.deallocate(m_data, m_capacity);
        m_data = NULL;
        m_capacity = 0;
        m_size = 0;
        m_offset = 0;
    }
}

void HJBuffer::setData(const uint8_t* data, size_t size)
{
    if (m_capacity < size) {
        realloc(size);
    }
    memcpy(m_data, data, size);
    m_size = size;
}

void HJBuffer::appendData(const uint8_t* data, size_t size)
{
    size_t totalSize = m_size + size;
    if (m_capacity < totalSize) {
        realloc(totalSize, true);
    }
    memcpy(m_data + m_size, data, size);
    m_size = totalSize;
}

NS_HJ_END
