#pragma once

#include "HJMediaUtils.h"
#include "HJPrerequisites.h"
#include "HJSPBuffer.h"
#include <deque>
//#include "HJTime.h"
//#include "HJFLog.h"

NS_HJ_BEGIN

class HJBaseMediaData 
{
public:
	HJ_DEFINE_CREATE(HJBaseMediaData);
    HJBaseMediaData() = default;
	virtual ~HJBaseMediaData() = default;

    HJSPBuffer::Ptr m_buffer = nullptr;
	int m_width = 0;
	int m_height = 0;
    int m_scaleWidth = 0;
    int m_scaleHeight = 0;
    int m_nSize = 0;
};

class HJRGBAMediaData : public HJBaseMediaData
{
public:
	HJ_DEFINE_CREATE(HJRGBAMediaData);
    HJRGBAMediaData() = default;
	virtual ~HJRGBAMediaData() = default;
    
    HJRGBAMediaData(int i_width, int i_height, unsigned char *i_data, int i_stride, int bufferSize)
        : HJRGBAMediaData(i_width, i_height, i_width, i_height, i_data, i_stride, bufferSize)
    {
    }
    HJRGBAMediaData(int i_width, int i_height, int i_scaleWidth, int i_scaleHeight, unsigned char *i_data, int i_stride, int bufferSize)
    {
        m_width = i_width;
        m_height = i_height;
        m_scaleWidth = i_scaleWidth;
        m_scaleHeight = i_scaleHeight;
        m_nSize = i_scaleWidth * i_scaleHeight * 4;
        m_buffer = HJSPBuffer::create(m_nSize);
        unsigned char *pData = m_buffer->getBuf();
        unsigned char *src = i_data;
        for (int i = 0; i < i_scaleHeight; i++) 
        {
            memcpy(pData, src, i_scaleWidth * 4);
            pData += i_scaleWidth * 4;
            src += i_stride;
        }
    }
    HJRGBAMediaData(HJSPBuffer::Ptr i_buffer, int i_width, int i_height)
        : HJRGBAMediaData(std::move(i_buffer), i_width, i_height, i_width, i_height)
    {
    }
    HJRGBAMediaData(HJSPBuffer::Ptr i_buffer, int i_width, int i_height, int i_scaleWidth, int i_scaleHeight)
    {
        m_width = i_width;
        m_height = i_height;
        m_scaleWidth = i_scaleWidth;
        m_scaleHeight = i_scaleHeight;
        m_nSize = i_scaleWidth * i_scaleHeight * 4;
        m_buffer = std::move(i_buffer);
    }
};
using HJMediaDataRGBAQueue = std::deque<HJRGBAMediaData::Ptr>;
class HJRawImageDataInfo
{
public:
    HJ_DEFINE_CREATE(HJRawImageDataInfo);
    HJSPBuffer::Ptr m_buffer = nullptr;
    int m_width = 0;
    int m_height = 0;
    int m_components = 0;
    int m_imgIdx = 0;
};


NS_HJ_END

