#pragma once


#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJSPBuffer.h"
#include "HJConvertUtils.h"

NS_HJ_BEGIN

//typedef enum HJTransferMediaDataType
//{
//    HJTransferMediaDataType_Unknown = -1,
//    HJTransferMediaDataType_YUVI420,
//    HJTransferMediaDataType_RGBA,
//} HJRenderMediaType;

class HJTransferMediaData;
using HJTransferMediaDataGetCb = std::function<void (std::shared_ptr<HJTransferMediaData> &o_data)>;
using HJTransferMediaDataSetCb = std::function<void(std::shared_ptr<HJTransferMediaData> i_data)>;

typedef enum HJTransferMediaStorageMode
{
    HJTransferMediaStorageMode_Owning = 0,
    // View data borrows upstream planes and must not cross async/cache boundaries.
    HJTransferMediaStorageMode_View = 1,
} HJTransferMediaStorageMode;

class HJTransferMediaData : public HJBaseParam
{
public:
	HJ_DEFINE_CREATE(HJTransferMediaData);
    HJTransferMediaData() = default;
    virtual ~HJTransferMediaData() = default;
    HJTransferMediaData(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0)
        : m_width(i_width), m_height(i_height), m_timeStamp(i_timestamp), m_storageMode(HJTransferMediaStorageMode_View)
    {
        for (int i = 0; i < 4; i++)
        {
            m_data[i] = i_data[i];
            m_stride[i] = i_stride[i];
        }
    }
    using Base = std::unordered_map<std::string, std::any>;
    void copymap(const Base& other) {
        this->clear();
        this->reserve(other.size()); 

        for (const auto& kv : other) {
            this->emplace(kv.first, kv.second);
        }
    }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    void setWidth(int width) { m_width = width; }
    void setHeight(int height) { m_height = height; }

    void setBuffer(HJSPBuffer::Ptr buffer) { m_buffer = buffer; }
    HJSPBuffer::Ptr getBuffer() const { return m_buffer; }
    void setStorageMode(HJTransferMediaStorageMode mode) { m_storageMode = mode; }
    HJTransferMediaStorageMode getStorageMode() const { return m_storageMode; }
    bool isView() const { return m_storageMode == HJTransferMediaStorageMode_View; }
    bool isOwning() const { return m_storageMode == HJTransferMediaStorageMode_Owning; }

    void setStride(int index, int stride)
    {
        if (index >= (sizeof(m_stride) / sizeof(m_stride[0])))
        {
            return;
        }
        m_stride[index] = stride;
    }

    int getStride(int index) const 
    { 
        if (index >= (sizeof(m_stride) / sizeof(m_stride[0])))
        {
            return -1;
        }
        return m_stride[index];
    }
    unsigned char* getData(int index) const
    {
        if (index >= (sizeof(m_data) / sizeof(m_data[0])))
        {
            return nullptr;
        }
        return m_data[index];
    }
    void setData(int index, unsigned char *i_data)
    {
        if (index >= (sizeof(m_data) / sizeof(m_data[0])))
        {
            return;
        }
        m_data[index] = i_data;
    }

    int64_t getTimeStamp() const { return m_timeStamp; }
    void setTimeStamp(int64_t timeStamp) { m_timeStamp = timeStamp; }

    void setConvertType(HJConvertDataType type) { m_type = type; }
    HJConvertDataType getConvertType() const { return m_type; }

    void set(HJConvertDataType i_type, int i_width = 0, int i_height = 0, int64_t i_timestamp = 0)
    {
        m_type = i_type;
        m_width = i_width;
        m_height = i_height;
        m_timeStamp = i_timestamp;
    }

    static HJTransferMediaData::Ptr create(HJConvertDataType i_srcType, unsigned char* i_data[4], int i_pitch[4], int i_width, int i_height, int64_t i_timestamp, HJConvertDataType i_dstType);
    static HJTransferMediaData::Ptr create(HJTransferMediaData::Ptr i_data, HJConvertDataType i_dstType);
    static HJTransferMediaData::Ptr makeOwning(const HJTransferMediaData::Ptr& src);

protected:
    int m_width = 0;
    int m_height = 0;
    HJSPBuffer::Ptr m_buffer = nullptr;
    int m_stride[4] = {0};
    unsigned char* m_data[4] = {nullptr};
    int64_t m_timeStamp = 0;
    HJConvertDataType m_type = HJConvertDataType_None;
    HJTransferMediaStorageMode m_storageMode = HJTransferMediaStorageMode_Owning;
};

class HJTransferMediaDataYUVI420 : public HJTransferMediaData
{
public:
    HJ_DEFINE_CREATE(HJTransferMediaDataYUVI420);
    static HJTransferMediaDataYUVI420::Ptr CreateView(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0);
    HJTransferMediaDataYUVI420();
    virtual ~HJTransferMediaDataYUVI420() = default;
    HJTransferMediaDataYUVI420(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0);
};

class HJTransferMediaDataRGBA : public HJTransferMediaData
{
public:
    HJ_DEFINE_CREATE(HJTransferMediaDataRGBA);
    static HJTransferMediaDataRGBA::Ptr CreateView(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0);
    HJTransferMediaDataRGBA();
    virtual ~HJTransferMediaDataRGBA() = default;
    HJTransferMediaDataRGBA(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0);
};
class HJTransferMediaDataRGB : public HJTransferMediaData
{
public:
    HJ_DEFINE_CREATE(HJTransferMediaDataRGB);
    static HJTransferMediaDataRGB::Ptr CreateView(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0);
    HJTransferMediaDataRGB();
    virtual ~HJTransferMediaDataRGB() = default;
    HJTransferMediaDataRGB(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0);
};
class HJTransferMediaDataYUVNV12 : public HJTransferMediaData
{
public:
    HJ_DEFINE_CREATE(HJTransferMediaDataYUVNV12);
    static HJTransferMediaDataYUVNV12::Ptr CreateView(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0);
    HJTransferMediaDataYUVNV12();
    virtual ~HJTransferMediaDataYUVNV12() = default;
    HJTransferMediaDataYUVNV12(unsigned char* i_data[4], int i_stride[4], int i_width, int i_height, int64_t i_timestamp = 0);
};

NS_HJ_END



