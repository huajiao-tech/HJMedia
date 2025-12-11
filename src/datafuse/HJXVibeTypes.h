//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once

#include <vector>
#include <cstdint>
#include <utility>
#include "HJUtilitys.h"
#include "HJAny.h"

NS_HJ_BEGIN
#define HJ_SEI_SEG_MAX_SIZE     4 * 1024
//***********************************************************************************//
enum class HJCompressType {
    COMPRESS_NONE = 0,
    COMPRESS_LZ4 = 1,
    COMPRESS_ZLIB = 2,
    // COMPRESS_ZSTD = 3,
    COMPRESS_MAX = 3,
};

enum class HJXVibDataType {
    VIBDATA_RAW = 0,
    VIBDATA_JSON = 1,
    VIBDATA_MAX = 2,
};

#define HJXVIB_VERSION      1
#define HJXVIB_HEADER_SIZE  10 //(sizeof(HJXVibHeader))
#define HJXVIB_MAGIC_NUMBER 0x1F
struct HJXVibHeader {
    uint8_t version = HJXVIB_VERSION;
    HJCompressType compress = HJCompressType::COMPRESS_NONE;    //2bit
    HJXVibDataType data_type = HJXVibDataType::VIBDATA_RAW;     //2bit
    uint8_t reserved = 0;
    uint8_t data_offset = 10;  // header size
    uint32_t data_len = 0;     // original uncompressed total length
    uint8_t seg_id = 0;        // 0..seg_count-1
    uint8_t seg_count = 1;     // total segments
    uint8_t seq_id = 0;        // sequence id provided by caller
};

class HJUserData : public HJObject 
{
public:
    HJ_DECLARE_PUWTR(HJUserData);

    HJUserData() = default;
    HJUserData(const HJRawBuffer& data) : m_data(data) { 
        m_originSize = m_data.size();
    }

    void setData(const HJRawBuffer& data) {
        m_data = data;
    }
    const HJRawBuffer& getData() const {
        return m_data;
    }
    const uint8_t* data() const { return m_data.data(); }
    size_t size() const { return m_data.size(); }

    void append(const HJRawBuffer& data) {
        m_data.insert(m_data.end(), data.begin(), data.end());
    }

    void setOriginSize(size_t size) {
        m_originSize = size;
    }
    size_t getOriginSize() const {
        return m_originSize;
    }
public:
   static HJUserData::Ptr make(const HJRawBuffer& data) {
        return HJCreates<HJUserData>(data);
    }
    static HJUserData::Ptr make(const uint8_t* data, size_t size) {
        auto ret = HJCreates<HJUserData>();
        ret->setData(HJRawBuffer(data, data + size));
        return ret;
    }
private:
    HJRawBuffer m_data;
    size_t m_originSize = 0;
};

class HJVibeData : public HJKeyStorage, public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJVibeData);

    HJVibeData() = default;
    HJVibeData(const HJRawBuffer& data) {
        append(HJUserData::make(data));
    }
    void setData(const std::vector<HJUserData::Ptr>& data) {
        m_data = data;
    }
    const std::vector<HJUserData::Ptr>& getData() const {
        return m_data;
    }
    //const HJUserData::Ptr& operator[](size_t i) const {
    //    return m_data[i];
    //}
    const HJUserData::Ptr& get(size_t i) const {
        return m_data[i];
    }
    size_t size() const {
        return m_data.size();
    }
    void append(const HJUserData::Ptr& data) {
        m_data.push_back(data);
    }
    void append(const std::vector<HJUserData::Ptr>& datas) {
        m_data.insert(m_data.end(), datas.begin(), datas.end());
    }
    void clear() {
        m_data.clear();
    }
public:
    static int64_t parseVibeSeqID(const uint8_t* data, size_t size) {
        if (size < HJXVIB_HEADER_SIZE) return -1;
        return data[9];
    }
    static int64_t parseVibeSeqID(const HJRawBuffer& data) {
        return parseVibeSeqID(data.data(), data.size());
    }

    //static bool verifyXVibHeader(const uint8_t* data, size_t size) {
    //    if (!data || size < HJXVIB_HEADER_SIZE) return false;
    //    if (HJXVIB_VERSION != data[0]) return false;
    //    if (data[1] >= (uint8_t)HJCompressType::COMPRESS_MAX) return false;
    //    if (data[2] >= (uint8_t)HJXVibDataType::VIBDATA_MAX) return false;
    //    if (0 != data[3]) return false;
    //    if (HJXVIB_HEADER_SIZE != data[4]) return false;
    //    if (data[9] >= data[10]) return false;
    //    return true;
    //}
    static bool verifyXVibHeader(const uint8_t* data, size_t size);
    static bool verifyXVibHeader(const HJRawBuffer& data) {
        return verifyXVibHeader(data.data(), data.size());
    }
private:
    std::vector<HJUserData::Ptr> m_data;
};

NS_HJ_END

