//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaInfo.h"
#include "HJMediaFrame.h"

struct AVal;

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJFLVPacketPriority
{
    HJFLV_PKT_PRIORITY_NONE = 0,
    HJFLV_PKT_PRIORITY_LOW  = 1,
    HJFLV_PKT_PRIORITY_MEDIUM = 2,
    HJFLV_PKT_PRIORITY_HIGH = 3,
} HJFLVPacketPriority;

class HJFLVPacket : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJFLVPacket>;
    /**
    * tsOffset : ms
    */
    int init(HJMediaFrame::Ptr frame, int64_t tsOffset = 0);

    const HJMediaType getType() const {
        return m_type;
    }
    const bool isVideo() const {
        return (HJMEDIA_TYPE_VIDEO == m_type);
    }
    const bool isAudio() const {
        return (HJMEDIA_TYPE_AUDIO == m_type);
    }
    const bool isKeyVFrame() const {
        if (isAudio()) {
            return false;
        }
        return m_keyFrame || (3 == m_priority);
    }
    const int getPriority() const {
        return m_priority;
    }
    const size_t getSize() const {
        return m_data ? m_data->size() : 0;
    }
    static HJFLVPacket::Ptr makeAudioPacket();
    static HJFLVPacket::Ptr makeVideoPacket(int codecid, bool isKeyFrame = false);

    std::string formatInfo();
public:
    HJMediaType     m_type = HJMEDIA_TYPE_NONE;
    int             m_codecid = 0;
    HJBuffer::Ptr   m_data = nullptr;
    int64_t         m_pts = 0;
    int64_t         m_dts = 0;
    int64_t         m_dtsUsec = 0;
    int64_t         m_dtsSysUsec = 0;
    int64_t         m_extraTS = HJ_NOTS_VALUE;      //capture timestamp
    int64_t         m_fileOffset = 0;
    bool            m_keyFrame = false;
    int             m_priority = HJFLV_PKT_PRIORITY_NONE;
    size_t          m_trackIdx = 0;
};
using HJFLVPacketList = HJList<HJFLVPacket::Ptr>;

class HJFLVPacketQueue : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJFLVPacketQueue>;

    int push(HJFLVPacket::Ptr packet);
    HJFLVPacket::Ptr pop();
    HJFLVPacket::Ptr front();
    HJFLVPacket::Ptr findFirstVideoPacket();
    //size_t drop(int priority);
    size_t size();
protected:
    HJFLVPacketList            m_packets;
    std::recursive_mutex        m_mutex;
};

class HJFLVUtils : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJFLVUtils>;

    static HJBuffer::Ptr buildMetaDataTag(HJMediaInfo::Ptr mediaInfo, bool writeHeader = false);
    static HJBuffer::Ptr buildMetaData(HJMediaInfo::Ptr mediaInfo);
    //audio track 1,....
    static HJBuffer::Ptr buildAdditionalMetaTag();
    static HJBuffer::Ptr buildAdditionalMetaData();

    static HJBuffer::Ptr buildPacket(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader);
    static HJBuffer::Ptr buildAdditionalPacket(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader, size_t index);
    // Enhanced RTMP (V2) Y2023 spec
    static HJBuffer::Ptr buildPacketEx(HJFLVPacket::Ptr packet, int64_t dts0ffset, int codecID, int type);
    static HJBuffer::Ptr buildPacketStart(HJFLVPacket::Ptr packet, int codecID);
    static HJBuffer::Ptr buildPacketFrames(HJFLVPacket::Ptr packet, int codecID, int64_t dts0ffset);
    static HJBuffer::Ptr buildPacketEnd(HJFLVPacket::Ptr packet, int codecID);

    static HJBuffer::Ptr buildPacketMetadata(int codecID, int bits_per_raw_sample,
        uint8_t color_primaries, int color_trc,
        int color_space, int min_luminance,
        int max_luminance);
protected:
    static HJBuffer::Ptr buildAudioTag(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader);
    static HJBuffer::Ptr buildAdditionalAudioData(HJFLVPacket::Ptr packet, bool isHeader, size_t index);
    static HJBuffer::Ptr buildAdditionalAudioTag(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader, size_t index);
    static HJBuffer::Ptr buildVideoTag(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader);

    static size_t getADTSHeaderLength(const uint8_t* data, size_t length);
private:
    static void wu29(HJBuffer::Ptr buffer, uint32_t val);
    static void wu29bValue(HJBuffer::Ptr buffer, uint32_t val);
    static void w4cc(HJBuffer::Ptr buffer, int codecID);
    //rtmp-helpers.h
    static AVal* flv_str(AVal* out, const char* str);
    static void enc_num_val(char** enc, char* end, const char* name, double val);
    static void enc_bool_val(char** enc, char* end, const char* name, bool val);
    static void enc_str_val(char** enc, char* end, const char* name, const char* val);
    static void enc_str(char** enc, char* end, const char* str);
};

NS_HJ_END