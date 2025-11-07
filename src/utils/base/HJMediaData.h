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
    int m_nSize = 0;
};

class HJRGBAMediaData : public HJBaseMediaData
{
public:
	HJ_DEFINE_CREATE(HJRGBAMediaData);
    HJRGBAMediaData() = default;
	virtual ~HJRGBAMediaData() = default;
    
    HJRGBAMediaData(int i_width, int i_height, unsigned char *i_data, int i_stride, int bufferSize)
    {
        m_width = i_width;
        m_height = i_height;
        m_nSize = i_width * i_height * 4;
        m_buffer = HJSPBuffer::create(m_nSize);
        unsigned char *pData = m_buffer->getBuf();
        unsigned char *src = i_data;
//        int64_t t0 = HJCurrentSteadyMS();
        for (int i = 0; i < i_height; i++) 
        {
            memcpy(pData, src, i_width * 4);
            pData += i_width * 4;
            src += i_stride;
        }
//        int64_t t1 = HJCurrentSteadyMS();
//        bufferSize = 921600;
//        HJSPBuffer::Ptr totalBuffer = HJSPBuffer::create(bufferSize);
//        memcpy(totalBuffer->getBuf(), i_data, bufferSize);
//        int64_t t2 = HJCurrentSteadyMS();
//        HJFLogi("test mempcy buffersize:{} :{} {}", bufferSize, (t1 - t0), (t2 - t1));
    }
    HJRGBAMediaData(HJSPBuffer::Ptr i_buffer, int i_width, int i_height)
    {
        m_width = i_width;
        m_height = i_height;
        m_nSize = i_width * i_height * 4;
        m_buffer = std::move(i_buffer);
    }
};
using HJMediaDataRGBAQueue = std::deque<HJRGBAMediaData::Ptr>;


class HJFacePointsWrapper
{
public:
	HJ_DEFINE_CREATE(HJFacePointsWrapper);
    HJFacePointsWrapper() = default;
	virtual ~HJFacePointsWrapper() = default;    
    
    HJFacePointsWrapper(int w, int h, std::string i_faceInfo)
    {
        m_width = w;
        m_height = h;
        m_faceInfo = i_faceInfo;
    }
    int m_width = 0;
    int m_height = 0;
    std::string m_faceInfo = "";
};


NS_HJ_END

