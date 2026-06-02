#include "HJTransferMediaData.h"
#include "HJFLog.h"
#include "libyuv.h"

NS_HJ_BEGIN

namespace
{
template <typename T>
static std::shared_ptr<T> createTransferMediaDataView(
    HJConvertDataType i_type,
    unsigned char* i_data[4],
    int i_stride[4],
    int i_width,
    int i_height,
    int64_t i_timestamp)
{
    std::shared_ptr<T> data = std::make_shared<T>();
    if (!data)
    {
        return nullptr;
    }

    data->set(i_type, i_width, i_height, i_timestamp);
    data->setBuffer(HJSPBuffer::Ptr());
    data->setStorageMode(HJTransferMediaStorageMode_View);
    for (int i = 0; i < 4; ++i)
    {
        data->setData(i, i_data[i]);
        data->setStride(i, i_stride[i]);
    }
    return data;
}

static HJTransferMediaData::Ptr cloneTransferMediaDataOwning(const HJTransferMediaData::Ptr& src)
{
    if (!src)
    {
        return nullptr;
    }

    unsigned char* data[4] = {
        src->getData(0),
        src->getData(1),
        src->getData(2),
        src->getData(3)
    };
    int stride[4] = {
        src->getStride(0),
        src->getStride(1),
        src->getStride(2),
        src->getStride(3)
    };

    HJTransferMediaData::Ptr dst = nullptr;
    switch (src->getConvertType())
    {
    case HJConvertDataType_RGB:
        dst = HJTransferMediaDataRGB::Create<HJTransferMediaDataRGB>(
            data, stride, src->getWidth(), src->getHeight(), src->getTimeStamp());
        break;
    case HJConvertDataType_RGBA:
        dst = HJTransferMediaDataRGBA::Create<HJTransferMediaDataRGBA>(
            data, stride, src->getWidth(), src->getHeight(), src->getTimeStamp());
        break;
    case HJConvertDataType_YUVNV12:
        dst = HJTransferMediaDataYUVNV12::Create<HJTransferMediaDataYUVNV12>(
            data, stride, src->getWidth(), src->getHeight(), src->getTimeStamp());
        break;
    case HJConvertDataType_YUV420:
        dst = HJTransferMediaDataYUVI420::Create<HJTransferMediaDataYUVI420>(
            data, stride, src->getWidth(), src->getHeight(), src->getTimeStamp());
        break;
    default:
        break;
    }

    if (dst)
    {
        dst->copymap(*src);
    }
    return dst;
}
}

HJTransferMediaData::Ptr HJTransferMediaData::create(HJConvertDataType i_srcType, unsigned char* i_data[4], int i_pitch[4], int i_width, int i_height, int64_t i_timestamp,
    HJConvertDataType i_dstType)
{
    HJTransferMediaData::Ptr data = nullptr;
    if (i_dstType == HJConvertDataType_YUVNV12)
    {
        if (i_srcType == HJConvertDataType_YUVNV12)
        {
            data = std::make_shared<HJTransferMediaDataYUVNV12>(i_data, i_pitch, i_width, i_height, i_timestamp);
            return data;
        }

        data = std::make_shared<HJTransferMediaDataYUVNV12>();
        HJSPBuffer::Ptr buffer = HJSPBuffer::create(i_width * i_height * 3 / 2);
        data->setBuffer(buffer);
        data->setData(0, buffer->getBuf());
        data->setData(1, data->m_data[0] + i_width * i_height);
        data->setStride(0, i_width); 
        data->setStride(1, i_width);
        data->setWidth(i_width);
        data->setHeight(i_height);
        data->setTimeStamp(i_timestamp);

        if(i_srcType == HJConvertDataType_RGB)
        {
            HJSPBuffer::Ptr RGBA = HJSPBuffer::create(i_width * i_height * 4);
            libyuv::RGB24ToARGB(i_data[0], i_pitch[0], RGBA->getBuf(), i_width * 4, i_width, i_height);

            libyuv::ABGRToNV12(RGBA->getBuf(), i_width * 4,
                data->getData(0), data->getStride(0), data->getData(1), data->getStride(1),
                data->getWidth(), data->getHeight());
        }
        else if(i_srcType == HJConvertDataType_RGBA)
        {
			libyuv::ABGRToNV12(i_data[0], i_pitch[0],
                data->getData(0), data->getStride(0), data->getData(1), data->getStride(1),
                data->getWidth(), data->getHeight());
        }
        else
        {
            data = nullptr;
        }
    }
    else if (i_dstType == HJConvertDataType_YUV420)
    {

    }
    else if (i_dstType == HJConvertDataType_RGBA)
    {
        if (i_srcType == HJConvertDataType_RGBA)
        {
            data = std::make_shared<HJTransferMediaDataRGBA>(i_data, i_pitch, i_width, i_height, i_timestamp);
            return data;
        }

        data = std::make_shared<HJTransferMediaDataRGBA>();
        HJSPBuffer::Ptr buffer = HJSPBuffer::create(i_width * i_height * 4);
        data->setBuffer(buffer);
        data->setData(0, buffer->getBuf());
        data->setStride(0, i_width * 4);
        data->setWidth(i_width);
        data->setHeight(i_height);
        data->setTimeStamp(i_timestamp);

        if(i_srcType == HJConvertDataType_YUVNV12)
        {    
            libyuv::NV12ToABGR(i_data[0], i_pitch[0], i_data[1], i_pitch[1],
                data->getData(0), data->getStride(0), data->getWidth(), data->getHeight());
        }   
        else if (i_srcType == HJConvertDataType_YUV420)
        {
            libyuv::I420ToABGR(i_data[0], i_pitch[0], i_data[1], i_pitch[1], i_data[2], i_pitch[2],
                data->getData(0), data->getStride(0), data->getWidth(), data->getHeight());
        }
        else if (i_srcType == HJConvertDataType_RGB)
        {
            //RAW(r,g,b) RAWToABGR is not have this function
            //libyuv::RAWToARGB(i_data[0], i_pitch[0],
            //    data->getData(0), data->getStride(0), data->getWidth(), data->getHeight());
            HJSPBuffer::Ptr RGBA = HJSPBuffer::create(i_width * i_height * 4);
            libyuv::RAWToARGB(i_data[0], i_pitch[0], RGBA->getBuf(), i_width * 4, i_width, i_height);
            libyuv::ARGBToABGR(RGBA->getBuf(), i_width * 4, data->getData(0), data->getStride(0), data->getWidth(), data->getHeight());
        }
        else
        {
            data = nullptr;
        }
    }
    else if (i_dstType == HJConvertDataType_RGB)
    {
        if (i_srcType == HJConvertDataType_RGB)
        {
            data = std::make_shared<HJTransferMediaDataRGB>(i_data, i_pitch, i_width, i_height, i_timestamp);
            return data;
		}
        else if (i_srcType == HJConvertDataType_RGBA)
        {
            data = std::make_shared<HJTransferMediaDataRGB>();
            HJSPBuffer::Ptr buffer = HJSPBuffer::create(i_width * i_height * 3);
            data->setBuffer(buffer);
            data->setData(0, buffer->getBuf());
            data->setStride(0, i_width * 3);
            data->setWidth(i_width);
            data->setHeight(i_height);
            data->setTimeStamp(i_timestamp);
            libyuv::ABGRToRAW(i_data[0], i_pitch[0], data->getData(0), data->getStride(0), data->getWidth(), data->getHeight()); //RAW(r,g,b)  RGB24(b,g,r)
        }
        else
        {
            data = nullptr;
		}
    }
    return data;
}
HJTransferMediaData::Ptr HJTransferMediaData::create(HJTransferMediaData::Ptr i_src, HJConvertDataType i_dstType)
{
    if (!i_src)
    {
        return nullptr;
    }

    if (i_src->getConvertType() == i_dstType)
    {
        return i_src->isOwning() ? i_src : makeOwning(i_src);
    }
    unsigned char* i_data[4] = {
        i_src->getData(0),
        i_src->getData(1),
        i_src->getData(2),
        i_src->getData(3)
    };
    int i_nPitch[4] = {
        i_src->getStride(0),
        i_src->getStride(1),
        i_src->getStride(2),
        i_src->getStride(3)
    };
    HJTransferMediaData::Ptr dst = create(i_src->getConvertType(), i_data, i_nPitch, i_src->getWidth(), i_src->getHeight(), i_src->getTimeStamp(), i_dstType);
    if (!dst)
    {
        return nullptr;
    }
    dst->copymap(*i_src);
    return dst;
}

HJTransferMediaData::Ptr HJTransferMediaData::makeOwning(const HJTransferMediaData::Ptr& src)
{
    if (!src)
    {
        return nullptr;
    }
    if (src->isOwning())
    {
        return src;
    }
    return cloneTransferMediaDataOwning(src);
}

HJTransferMediaDataYUVI420::Ptr HJTransferMediaDataYUVI420::CreateView(
    unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp)
{
    return createTransferMediaDataView<HJTransferMediaDataYUVI420>(
        HJConvertDataType_YUV420, i_data, i_stride, i_width, i_height, i_timestamp);
}

HJTransferMediaDataYUVI420::HJTransferMediaDataYUVI420()
{
    set(HJConvertDataType_YUV420);
}
HJTransferMediaDataYUVI420::HJTransferMediaDataYUVI420(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp)
{
    set(HJConvertDataType_YUV420, i_width, i_height, i_timestamp);
      
    m_buffer = HJSPBuffer::create(m_width * m_height * 3 / 2);

    m_data[0] = m_buffer->getBuf();
    m_data[1] = m_buffer->getBuf() + m_width * m_height;
    m_data[2] = m_buffer->getBuf() + m_width * m_height * 5 / 4;
    
    m_stride[0] = m_width;
    m_stride[1] = m_width / 2;
    m_stride[2] = m_width / 2;

    libyuv::I420Copy(i_data[0], i_stride[0], i_data[1], i_stride[1], i_data[2], i_stride[2],
        m_data[0], m_stride[0], m_data[1], m_stride[1], m_data[2], m_stride[2],
        m_width, m_height);
}

HJTransferMediaDataRGBA::Ptr HJTransferMediaDataRGBA::CreateView(
    unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp)
{
    return createTransferMediaDataView<HJTransferMediaDataRGBA>(
        HJConvertDataType_RGBA, i_data, i_stride, i_width, i_height, i_timestamp);
}

HJTransferMediaDataRGBA::HJTransferMediaDataRGBA()
{
    set(HJConvertDataType_RGBA);
}
HJTransferMediaDataRGBA::HJTransferMediaDataRGBA(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp)
{
    set(HJConvertDataType_RGBA, i_width, i_height, i_timestamp);

    m_buffer = HJSPBuffer::create(m_width * m_height * 4);

    m_data[0] = m_buffer->getBuf();
    m_stride[0] = m_width * 4;

    libyuv::ARGBCopy(i_data[0], i_stride[0], m_data[0], m_stride[0], m_width, m_height);
}

HJTransferMediaDataRGB::Ptr HJTransferMediaDataRGB::CreateView(
    unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp)
{
    return createTransferMediaDataView<HJTransferMediaDataRGB>(
        HJConvertDataType_RGB, i_data, i_stride, i_width, i_height, i_timestamp);
}

HJTransferMediaDataRGB::HJTransferMediaDataRGB()
{
    set(HJConvertDataType_RGB);
}
HJTransferMediaDataRGB::HJTransferMediaDataRGB(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp)
{
    set(HJConvertDataType_RGB, i_width, i_height, i_timestamp);


    m_buffer = HJSPBuffer::create(m_width * m_height * 3);
    m_data[0] = m_buffer->getBuf();
    m_stride[0] = m_width * 3;
    if (m_stride[0] == i_stride[0])
    {
        // If strides are the same, we can do a single memcpy
        memcpy(m_data[0], i_data[0], i_height * i_stride[0]);
    }
    else
    {
        // If strides are different, copy row by row
        for (int i = 0; i < i_height; ++i) 
        {
            memcpy(m_data[0] + i * m_stride[0], i_data[0] + i * i_stride[0], m_stride[0]);
        }
    }
}

HJTransferMediaDataYUVNV12::Ptr HJTransferMediaDataYUVNV12::CreateView(
    unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp)
{
    return createTransferMediaDataView<HJTransferMediaDataYUVNV12>(
        HJConvertDataType_YUVNV12, i_data, i_stride, i_width, i_height, i_timestamp);
}

HJTransferMediaDataYUVNV12::HJTransferMediaDataYUVNV12()
{
    set(HJConvertDataType_YUVNV12);
}
HJTransferMediaDataYUVNV12::HJTransferMediaDataYUVNV12(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp)
{
    set(HJConvertDataType_YUVNV12, i_width, i_height, i_timestamp);

    m_buffer = HJSPBuffer::create(m_width * m_height * 3 / 2);

    m_data[0] = m_buffer->getBuf();
    m_data[1] = m_data[0] + m_width * m_height;

    m_stride[0] = m_width;
    m_stride[1] = m_width;

    libyuv::NV12Copy(i_data[0], i_stride[0], i_data[1], i_stride[1], m_data[0], m_stride[0], m_data[1], m_stride[1], m_width, m_height);
}


NS_HJ_END
