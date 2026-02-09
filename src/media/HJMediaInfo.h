//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <set>
#include "HJUtilitys.h"
#include "HJMediaUtils.h"

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVChannelLayout AVChannelLayout;

#define HJ_FRAMERATE_DEFALUT   30          //fps
#define HJ_INTERVAL_DEFAULT    100         //ms

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJMediaType
{
    HJMEDIA_TYPE_NONE       = 0x0000,
    HJMEDIA_TYPE_VIDEO      = 0x0001,
    HJMEDIA_TYPE_AUDIO      = 0x0002,
    HJMEDIA_TYPE_DATA       = 0x0004,
    HJMEDIA_TYPE_SUBTITLE   = 0x0008,
    HJMEDIA_TYPE_ATTACHMENT = 0x0010,
    HJMEDIA_TYPE_AV         = HJMEDIA_TYPE_VIDEO | HJMEDIA_TYPE_AUDIO,
    HJMEDIA_TYPE_NB         = HJMEDIA_TYPE_VIDEO | HJMEDIA_TYPE_AUDIO | HJMEDIA_TYPE_SUBTITLE | HJMEDIA_TYPE_DATA
} HJMediaType;

typedef enum HJDataType
{
    HJDATA_TYPE_NONE = 0,
    HJDATA_TYPE_ES,       //encoded stream (AVC, HEVC, AAC...)
    HJDATA_TYPE_RS,       //raw stream (YUV, PCM....)
} HJDataType;

typedef enum HJQuality
{
    HJQuality_Default = 0,
    HJQuality_MD,
    HJQuality_HQ,
} HJQuality;

typedef enum HJ_NAL_TYPE
{
    HJ_NAL_UNKNOWN = 0,
    HJ_NAL_SLICE = 1,
    HJ_NAL_SLICE_DPA = 2,
    HJ_NAL_SLICE_DPB = 3,
    HJ_NAL_SLICE_DPC = 4,
    HJ_NAL_SLICE_IDR = 5,
    HJ_NAL_SEI = 6,
    HJ_NAL_SPS = 7,
    HJ_NAL_PPS = 8,
    HJ_NAL_AUD = 9,
    HJ_NAL_FILLER = 12,
} HJ_NAL_TYPE;

typedef enum HJ_HEVC_NAL_TYPE 
{
    HJ_HEVC_NAL_TRAIL_N = 0,
    HJ_HEVC_NAL_TRAIL_R = 1,
    HJ_HEVC_NAL_TSA_N = 2,
    HJ_HEVC_NAL_TSA_R = 3,
    HJ_HEVC_NAL_STSA_N = 4,
    HJ_HEVC_NAL_STSA_R = 5,
    HJ_HEVC_NAL_RADL_N = 6,
    HJ_HEVC_NAL_RADL_R = 7,
    HJ_HEVC_NAL_RASL_N = 8,
    HJ_HEVC_NAL_RASL_R = 9,
    HJ_HEVC_NAL_VCL_N10 = 10,
    HJ_HEVC_NAL_VCL_R11 = 11,
    HJ_HEVC_NAL_VCL_N12 = 12,
    HJ_HEVC_NAL_VCL_R13 = 13,
    HJ_HEVC_NAL_VCL_N14 = 14,
    HJ_HEVC_NAL_VCL_R15 = 15,
    HJ_HEVC_NAL_BLA_W_LP = 16,
    HJ_HEVC_NAL_BLA_W_RADL = 17,
    HJ_HEVC_NAL_BLA_N_LP = 18,
    HJ_HEVC_NAL_IDR_W_RADL = 19,
    HJ_HEVC_NAL_IDR_N_LP = 20,
    HJ_HEVC_NAL_CRA_NUT = 21,
    HJ_HEVC_NAL_RSV_IRAP_VCL22 = 22,
    HJ_HEVC_NAL_RSV_IRAP_VCL23 = 23,
    HJ_HEVC_NAL_RSV_VCL24 = 24,
    HJ_HEVC_NAL_RSV_VCL25 = 25,
    HJ_HEVC_NAL_RSV_VCL26 = 26,
    HJ_HEVC_NAL_RSV_VCL27 = 27,
    HJ_HEVC_NAL_RSV_VCL28 = 28,
    HJ_HEVC_NAL_RSV_VCL29 = 29,
    HJ_HEVC_NAL_RSV_VCL30 = 30,
    HJ_HEVC_NAL_RSV_VCL31 = 31,
    HJ_HEVC_NAL_VPS = 32,
    HJ_HEVC_NAL_SPS = 33,
    HJ_HEVC_NAL_PPS = 34,
    HJ_HEVC_NAL_AUD = 35,
    HJ_HEVC_NAL_EOS_NUT = 36,
    HJ_HEVC_NAL_EOB_NUT = 37,
    HJ_HEVC_NAL_FD_NUT = 38,
    HJ_HEVC_NAL_SEI_PREFIX = 39,
    HJ_HEVC_NAL_SEI_SUFFIX = 40,
} HJ_HEVC_NAL_TYPE;
//***********************************************************************************//
class HJUtilTime
{
public:
    using Ptr = std::shared_ptr<HJUtilTime>;
    HJUtilTime(int64_t value)
        : m_value(value)
        , m_timeBase(HJ_TIMEBASE_MS)
    {
        if (m_timeBase.den) {
            m_sec = m_value * m_timeBase.num / (double)m_timeBase.den;
        }
    }
    HJUtilTime(int64_t value, HJTimeBase tbase)
        : m_value(value)
        , m_timeBase(tbase)
    {
        if (m_timeBase.den) {
            m_sec = m_value * m_timeBase.num / (double)m_timeBase.den;
        }
    }
    HJUtilTime(int64_t value, int tbase)
        : m_value(value)
        , m_timeBase({1, tbase})
    {
        if (m_timeBase.den) {
            m_sec = m_value * m_timeBase.num / (double)m_timeBase.den;
        }
    }
    void setValue(int64_t value) {
        m_value = value;
        if (m_timeBase.den) {
            m_sec = m_value * m_timeBase.num / (double)m_timeBase.den;
        }
    }
    const int64_t getValue() {
        return m_value;
    }
    void addOffset(int64_t offset) {
        setValue(m_value + offset);
    }
    const HJTimeBase& getTimeBase() {
        return m_timeBase;
    }
    const double getSecond() {
        return m_sec;
    }
    const int64_t getMS() {
        return HJ_SEC_TO_MS(m_sec);
    }
    const int64_t getUS() {
        return HJ_SEC_TO_US(m_sec);
    }
    HJUtilTime& operator+=(HJUtilTime& other)
    {
        if (other.isInvalid()) {
            return *this;
        }
        auto& otbase = other.getTimeBase();
        if (isBaseEqual(otbase)) {
            m_value += other.getValue();
        }
        else {
            HJUtilTime unt = other.unified(m_timeBase);
            m_value += unt.getValue();
        }
        if (m_timeBase.den) {
            m_sec = m_value * m_timeBase.num / (double)m_timeBase.den;
        }
        return *this;
    }
    HJUtilTime operator+(HJUtilTime& other)
    {
        if (other.isInvalid()) {
            return *this;
        }
        int64_t val = 0;
        auto& otbase = other.getTimeBase();
        if (isBaseEqual(otbase)) {
            val = m_value + other.getValue();
        } else {
            HJUtilTime unt = other.unified(m_timeBase);
            val = m_value + unt.getValue();
        }
        return HJUtilTime(val, m_timeBase);
    }
    HJUtilTime& operator-=(HJUtilTime& other)
    {
        if (other.isInvalid()) {
            return *this;
        }
        auto& otbase = other.getTimeBase();
        if (isBaseEqual(otbase)) {
            m_value -= other.getValue();
        }
        else {
            HJUtilTime unt = other.unified(m_timeBase);
            m_value -= unt.getValue();
        }
        if (m_timeBase.den) {
            m_sec = m_value * m_timeBase.num / (double)m_timeBase.den;
        }
        return *this;
    }
    HJUtilTime operator-(HJUtilTime& other)
    {
        if (other.isInvalid()) {
            return *this;
        }
        int64_t val = 0;
        auto& otbase = other.getTimeBase();
        if (isBaseEqual(otbase)) {
            val = m_value - other.getValue();
        } else {
            HJUtilTime unt = other.unified(m_timeBase);
            val = m_value - unt.getValue();
        }
        return HJUtilTime(val, m_timeBase);
    }
    bool operator==(HJUtilTime& other) {
        if (other.isInvalid()) {
            return false;
        }
        auto& otbase = other.getTimeBase();
        if (isBaseEqual(otbase)) {
            if (m_value == other.getValue()) {
                return true;
            }
        } else {
            HJUtilTime unt = other.unified(m_timeBase);
            if (m_value == unt.getValue()) {
                return true;
            }
        }
        return false;
    }

    HJUtilTime unified(const HJTimeBase& tbase);
    //HJUtilTime unified(HJTimeBase& tbase) {
    //    int64_t b = m_timeBase.num * tbase.den;
    //    int64_t c = m_timeBase.den * tbase.num;
    //    int64_t a = m_value * b / c;
    //    return HJUtilTime(a, tbase);
    //}

    const bool isBaseEqual(const HJTimeBase& tbase) {
        if (m_timeBase.num == tbase.num && m_timeBase.den == tbase.den) {
            return true;
        }
        return false;
    }
    const bool isInvalid() {
        return (!m_timeBase.num || !m_timeBase.den);
    }
public:
    static const HJUtilTime HJTimeZero;
    static const HJUtilTime HJTimeNOTS;
    static const HJUtilTime HJTimeInvalid;
private:
    int64_t     m_value = 0;
    HJTimeBase m_timeBase = HJ_TIMEBASE_MS;    //ms
    double      m_sec = 0.0;
};

class HJTimeRange
{
public:
    using Ptr = std::shared_ptr<HJTimeRange>;
    HJTimeRange(HJUtilTime start, HJUtilTime duration)
        : m_start(std::move(start))
        , m_duration(std::move(duration)) 
    {
        m_end = m_start + m_duration;
    }
    void setStart(HJUtilTime start) {
        m_start = std::move(start);
    }
    HJUtilTime& getStart() {
        return m_start;
    }
    void setEnd(HJUtilTime end) {
        m_end = std::move(end);
    }
    HJUtilTime& getEnd() {
        return m_end;
    }
    void setDuration(HJUtilTime duration) {
        m_duration = std::move(duration);
    }
    HJUtilTime& getDuration() {
        return m_duration;
    }
private:
    HJUtilTime     m_start = HJUtilTime::HJTimeZero;
    HJUtilTime     m_end = HJUtilTime::HJTimeZero;
    HJUtilTime     m_duration = HJUtilTime::HJTimeZero;
};

class HJMediaTime
{
public:
    using Ptr = std::shared_ptr<HJMediaTime>;
    HJMediaTime(HJUtilTime pts, HJUtilTime dts)
        : m_pts(std::move(pts))
        , m_dts(std::move(dts))
    { }
    HJMediaTime(int64_t pts, int64_t dts, HJTimeBase tbase)
        : m_pts({ pts, tbase })
        , m_dts({ dts, tbase })
    { }
    HJMediaTime(HJUtilTime pts, HJUtilTime dts, HJUtilTime duration)
        : m_pts(std::move(pts))
        , m_dts(std::move(dts))
        , m_duration(std::move(duration))
    { }
    HJMediaTime(int64_t pts, int64_t dts, int64_t duration, HJTimeBase tbase)
        : m_pts({pts, tbase})
        , m_dts({dts, tbase})
        , m_duration({duration, tbase})
    { }
    HJMediaTime(int64_t pts, int64_t dts, int64_t duration, int tbase)
        : m_pts({pts, {1, tbase}})
        , m_dts({dts, {1, tbase}})
        , m_duration({duration, {1, tbase}})
    { }
    void setPTS(HJUtilTime pts) {
        m_pts = std::move(pts);
    }
    HJUtilTime& getPTS() {
        return m_pts;
    }
    const int64_t getPTSValue() {
        return m_pts.getValue();
    }
    const int64_t getPTSMS() {
        return m_pts.getMS();
    }
    const int64_t getPTSUS() {
        return m_pts.getUS();
    }
    void setDTS(HJUtilTime dts) {
        m_dts = std::move(dts);
    }
    HJUtilTime& getDTS() {
        return m_dts;
    }
    const int64_t getDTSMS() {
        return m_dts.getMS();
    }
    const int64_t getDTSUS() {
        return m_dts.getUS();
    }
    const int64_t getDTSValue() {
        return m_dts.getValue();
    }
    void setDuration(HJUtilTime duration) {
        m_duration = std::move(duration);
    }
    HJUtilTime& getDuration() {
        return m_duration;
    }
    const int64_t getDurationMS() {
        return m_duration.getMS();
    }
    const int64_t getDurationUS() {
        return m_duration.getUS();
    }
    HJMediaTime operator+(HJUtilTime& offset) {
        if (offset.isInvalid()) {
            return *this;
        }
        auto pts = m_pts + offset;
        auto dts = m_dts + offset;
        return HJMediaTime(pts, dts, m_duration);
    }
    HJMediaTime operator-(HJUtilTime& offset) {
        if (offset.isInvalid()) {
            return *this;
        }
        auto pts = m_pts - offset;
        auto dts = m_dts - offset;
        return HJMediaTime(pts, dts, m_duration);
    }
    HJMediaTime& operator+=(HJUtilTime& offset) {
        if (offset.isInvalid()) {
            return *this;
        }
        m_pts += offset;
        m_dts += offset;
        return *this;
    }
    HJMediaTime& operator-=(HJUtilTime& offset) {
        if (offset.isInvalid()) {
            return *this;
        }
        m_pts -= offset;
        m_dts -= offset;
        return *this;
    }
public:
    static const HJMediaTime HJMediaTimeZero;
    
protected:
    HJUtilTime     m_pts = HJUtilTime::HJTimeZero;
    HJUtilTime     m_dts = HJUtilTime::HJTimeZero;
    HJUtilTime     m_duration = HJUtilTime::HJTimeZero;
};
//***********************************************************************************//
//media time stamp(pts, dts, duration, time base)
class HJTimeStamp : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJTimeStamp>;
    HJTimeStamp(int64_t pts, int64_t dts, int64_t duration, HJTimeBase tbase = HJ_TIMEBASE_MS)
        : m_pts(pts)
        , m_dts(dts)
        , m_duration(duration)
        , m_timeBase(tbase)
    { }

    HJTimeStamp& operator+=(HJUtilTime& other)
    {
        if (other.isInvalid()) {
            return *this;
        }
        const auto offset = isBaseEqual(other.getTimeBase()) ? other.getValue() : other.unified(m_timeBase).getValue();
        m_pts += offset;
        m_dts += offset;
        m_duration += offset;
        return *this;
    }
    HJTimeStamp& operator+=(const int offset) {
        HJUtilTime delta(offset, HJ_TIMEBASE_MS);
        *this += delta;
        return *this;
    }
    HJTimeStamp operator+(HJUtilTime& other)
    {
        if (other.isInvalid()) {
            return *this;
        }
        const auto offset = isBaseEqual(other.getTimeBase()) ? other.getValue() : other.unified(m_timeBase).getValue();
        return HJTimeStamp(m_pts + offset, m_dts + offset, m_duration + offset, m_timeBase);
    }
    HJTimeStamp operator+(const int offset) {
        HJUtilTime delta(offset, HJ_TIMEBASE_MS);
        return *this + delta;
    }
    HJTimeStamp& operator-=(HJUtilTime& other)
    {
        if (other.isInvalid()) {
            return *this;
        }
        const auto offset = isBaseEqual(other.getTimeBase()) ? other.getValue() : other.unified(m_timeBase).getValue();
        m_pts -= offset;
        m_dts -= offset;
        m_duration -= offset;
        return *this;
    }
    HJTimeStamp& operator-=(const int offset) {
        HJUtilTime delta(offset, HJ_TIMEBASE_MS);
        *this -= delta;
        return *this;
    }
    HJTimeStamp operator-(HJUtilTime& other)
    {
        if (other.isInvalid()) {
            return *this;
        }
        const auto offset = isBaseEqual(other.getTimeBase()) ? other.getValue() : other.unified(m_timeBase).getValue();
        return HJTimeStamp(m_pts - offset, m_dts - offset, m_duration - offset, m_timeBase);
    }
    HJTimeStamp operator-(const int offset) {
        HJUtilTime delta(offset, HJ_TIMEBASE_MS);
        return *this - delta;
    }
    //ms
    const int64_t getPTS();
    const int64_t getDTS();
    const int64_t getDuration();
protected:
    const bool isBaseEqual(const HJTimeBase& tbase) {
        if (m_timeBase.num == tbase.num && m_timeBase.den == tbase.den) {
            return true;
        }
        return false;
    }
    const bool isInvalid() {
        return (!m_timeBase.num || !m_timeBase.den);
    }
protected:
    int64_t     m_pts = 0;
    int64_t     m_dts = 0;
    int64_t     m_duration = 0;
    HJTimeBase m_timeBase = HJ_TIMEBASE_MS;    //ms
};

//***********************************************************************************//
class HJMediaUrl : public HJKeyStorage, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJMediaUrl>;
    //
    HJMediaUrl(const HJMediaUrl::Ptr& other);
    HJMediaUrl(const std::string& url = "");
    HJMediaUrl(const std::string& url, int loopCnt, int64_t startTime = HJ_NOTS_VALUE, int64_t endTime = HJ_NOTS_VALUE, int disableMFlag = HJMEDIA_TYPE_NONE);
    virtual ~HJMediaUrl();
    
    void setUrl(const std::string& url) {
        m_url = url;
        m_urlHash = HJUtilitys::hash(m_url);
    }
    const std::string& getUrl() const {
        return m_url;
    }
    const size_t& getUrlHash() const {
        return m_urlHash;
    }
    void setOutUrl(const std::string& url) {
        m_outUrl = url;
        m_outUrlHash = HJUtilitys::hash(m_outUrl);
    }
    const std::string& getOutUrl() const {
        return m_outUrl;
    }
    const size_t& getOutUrlHash() const {
        return m_outUrlHash;
    }
    void setFmtName(const std::string& fmtName) {
        m_fmtName = fmtName;
    }
    const std::string& getFmtName() const {
        return m_fmtName;
    }
    void setStartTime(int64_t time) {
        m_startTime = time;
    }
    int64_t getStartTime() {
        return m_startTime;
    }
    void setEndTime(int64_t time) {
        m_endTime = time;
    }
    int64_t getEndTime() {
        return m_endTime;
    }
    void setDuration(int64_t duration) {
        m_duration = duration;
    }
    const int64_t getDuration() {
        return m_duration;
    }
    void setTimeout(const int64_t timeout) {
        m_timeout = timeout;
    }
    const int64_t getTimeout() const {
        return m_timeout;
    }
    void setLoopCnt(int loopCnt) {
        m_loopCnt = loopCnt;
    }
    int getLoopCnt() {
        return m_loopCnt;
    }
    void setDisableMFlag(const int flag) {
        m_disableMFlag = flag;
    }
    int getDisableMFlag() const {
        return m_disableMFlag;
    }
    void setMFlagTimeout(const int timeout) {
        m_mflagTimeout = timeout;
    }
    int64_t getMFlagTimeout() const {
        return m_mflagTimeout;
    }

    bool isEnbleVideo() const {
        return !(m_disableMFlag & HJMEDIA_TYPE_VIDEO);
    }
    bool isEnableAudio() const {
        return !(m_disableMFlag & HJMEDIA_TYPE_AUDIO);
    }
    bool isEnableData() const {
        return !(m_disableMFlag & HJMEDIA_TYPE_DATA);
    }
    bool isEnableSubtitle() const {
        return !(m_disableMFlag & HJMEDIA_TYPE_SUBTITLE);
    }
    bool isEnableAttachment() const {
        return !(m_disableMFlag & HJMEDIA_TYPE_ATTACHMENT);
    }
    void setUseFast(bool useFast) {
        m_useFast = useFast;
    }
    bool getUseFast() const {
        return m_useFast;
    }
    bool isM3u8() const {
        if(m_url.empty()) return false;
        return HJUtilitys::containWith(m_url, ".m3u8");
    }
    bool isRTMP() const {
        if(m_url.empty()) return false;
        return HJUtilitys::startWith(m_url, "rtmp");
    }
    bool canUseFast() const {
        return m_useFast && !isM3u8();
    }

    virtual void clone(const HJMediaUrl::Ptr& other) {
        m_url = other->m_url;
        m_urlHash = other->m_urlHash;
        m_outUrl = other->m_outUrl;
        m_outUrlHash = other->m_outUrlHash;
        m_fmtName = other->m_fmtName;
        m_startTime = other->m_startTime;
        m_endTime = other->m_endTime;
        m_duration = other->m_duration;
        m_timeout = other->m_timeout;
        m_loopCnt = other->m_loopCnt;
        m_disableMFlag = other->m_disableMFlag;
        m_useFast = other->m_useFast;
        m_subUrls = other->m_subUrls;
        //
        this->insert(other->begin(), other->end());
    }
    virtual HJMediaUrl::Ptr dup() {
        HJMediaUrl::Ptr url = std::make_shared<HJMediaUrl>();
        url->clone(sharedFrom(this));
        return url;
    }
    void addSubUrl(const HJMediaUrl::Ptr& url) {
        m_subUrls.insert(url);
    }
    const auto& getSubUrls() {
        return m_subUrls;
    }
public:
    static HJMediaUrl::Ptr makeMediaUrl(const std::string& url, int flag = HJMEDIA_TYPE_NONE);

    static const std::string STORE_KEY_SUBURLS;
protected:
    std::string     m_url = "";
    size_t          m_urlHash = 0;
    std::string     m_outUrl = "";
    size_t          m_outUrlHash = 0;
    std::string     m_fmtName = "";
    int64_t         m_startTime = HJ_NOTS_VALUE;        //ms
    int64_t         m_endTime = HJ_NOTS_VALUE;          //ms
    int64_t         m_duration = 0;                      //ms
    int64_t         m_timeout = 3000000;                 //us
    int             m_loopCnt = 1;                       //0=1, HJ_INT_MAX;
    int             m_disableMFlag = HJMEDIA_TYPE_NONE;  //disable video | audio | subtitle
    int64_t         m_mflagTimeout = 0;                  //ms
    bool            m_useFast = true;                    //use fast http or https
    //
    std::set<HJMediaUrl::Ptr>   m_subUrls;
};
using HJMediaUrlVector = std::vector<HJMediaUrl::Ptr>;
using HJMediaUrlSet = std::set<HJMediaUrl::Ptr>;

//***********************************************************************************//
class HJHWDevice
{
public:
    using Ptr = std::shared_ptr<HJHWDevice>;
    HJHWDevice(const HJDeviceType devType, int devID = 0);
    HJHWDevice(const HJHND hwCtx);
    virtual ~HJHWDevice();

    int init();
    const HJHND getHWDeviceCtx() const {
        return m_hwCtx;
    }
    const HJDeviceType getDeviceType() const {
        return m_deviceType;
    }
    const int getDeviceID() const {
        return m_deviceID;
    }
private:
    HJHND              m_hwCtx = NULL;
    HJDeviceType       m_deviceType = HJDEVICE_TYPE_NONE;
    int                 m_deviceID = 0;
};
using HJHWDeviceMap = std::unordered_map<std::string, HJHWDevice::Ptr>;

//***********************************************************************************//
class HJHWFrameCtx
{
public:
    using Ptr = std::shared_ptr<HJHWFrameCtx>;
    HJHWFrameCtx(const HJHND frameRef);
    HJHWFrameCtx(const HJHWDevice::Ptr dev, int pixFmt, int swFmt, int width, int height, int poolSize = 0);
    virtual ~HJHWFrameCtx();

    const HJHND getHWFrameRef() {
        return m_hwFrameRef;
    }
private:
    HJHND              m_hwFrameRef = NULL;
    int                 m_pixFmt = 0;
    int                 m_swFormat = 0;
    int                 m_width = 0;
    int                 m_height = 0;
    int                 m_poolSize = 0;
};
//***********************************************************************************//
class HJCodecParameters : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJCodecParameters>;
    HJCodecParameters();
    HJCodecParameters(const AVCodecParameters* codecParams, const bool isDup = false);
    HJCodecParameters(const HJCodecParameters::Ptr& other);
    virtual ~HJCodecParameters();

    AVCodecParameters* getAVCodecParameters() const {
        return m_codecParams;
    }
    virtual void clone(const HJCodecParameters::Ptr& other);
    virtual HJCodecParameters::Ptr dup() {
        HJCodecParameters::Ptr obj = std::make_shared<HJCodecParameters>();
        obj->clone(sharedFrom(this));
        return obj;
    }
    bool isEqual(const HJCodecParameters::Ptr& other);
private:
    AVCodecParameters* m_codecParams = NULL;
};

//***********************************************************************************//
class HJStreamInfo : public HJKeyStorage, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJStreamInfo>;
    HJStreamInfo();
    virtual ~HJStreamInfo();
    
    void setType(const HJMediaType type) {
        m_type = type;
    }
    const HJMediaType getType() const {
        return m_type;
    }
    void setSubType(const int subType) {
        m_subType = subType;
    }
    const int getSubType() const {
        return m_subType;
    }
    void setDataType(const HJDataType type) {
        m_dataType = type;
    }
    const HJDataType getDataType() const {
        return m_dataType;
    }
    void setCodecID(int codecID) {
        m_codecID = codecID;
    }
    const int getCodecID() const {
        return m_codecID;
    }
    void setCodecName(const std::string& name) {
        m_codecName = name;
    }
    const std::string& getCodecName() {
        return m_codecName;
    }
    void setBitrate(const int bitrate) {
        m_bitrate = bitrate;
    }
    const int getBitrate() const {
        return m_bitrate;
    }
    HJDeclareVariableFunction(HJTimeBase, TimeBase, HJ_TIMEBASE_9W);

    void setTimeRange(const HJTimeRange timeRange) {
        m_timeRange = timeRange;
    }
    const HJTimeRange getTimeRange() const {
        return m_timeRange;
    }
    void setDevType(const HJDeviceType devType) {
        m_devType = devType;
    }
    const HJDeviceType getDevType() const {
        return m_devType;
    }
    void setStreamIndex(size_t streamID) {
        m_streamIndex = streamID;
    }
    const size_t getStreamIndex() const {
        return m_streamIndex;
    }
    const std::string getTrackKey() {
        return HJStreamInfo::type2String(m_type) + HJ2STR(m_trackID);
    }
    virtual void clone(const HJStreamInfo::Ptr& other);
    virtual HJStreamInfo::Ptr dup() {
        HJStreamInfo::Ptr info = std::make_shared<HJStreamInfo>();
        info->clone(sharedFrom(this));
        return info;
    }

    HJCodecParameters::Ptr getCodecParams();
    void setCodecParams(const HJCodecParameters::Ptr codecParams);
    AVCodecParameters* getAVCodecParams();
    void setAVCodecParams(const AVCodecParameters* codecParams);

    HJHWDevice::Ptr getHWDevice();
    void setHWDevice(const HJHWDevice::Ptr device);
    
    virtual const std::string formatInfo();
public:
    static const std::string KEY_WORLDS;
    static const std::string STORE_KEY_AVCODECPARMS;        //AVCodec Params
    static const std::string STORE_KEY_HWDEVICE;
    //
    static const std::string type2String(int type);
    static AVCodecParameters* makeCodecParameters(const HJStreamInfo::Ptr& info);
public:
    HJMediaType     m_type = HJMEDIA_TYPE_NONE;
    int             m_subType = 0;
    HJDataType     m_dataType = HJDATA_TYPE_NONE;
    int             m_codecID = 0;
    std::string     m_codecName = "";
    std::string     m_codecDevName = "";
    std::string     m_cert = "";
    int             m_bitrate = 0;
    HJTimeRange    m_timeRange = {HJUtilTime::HJTimeZero, HJUtilTime::HJTimeZero};
    HJDeviceType   m_devType = HJDEVICE_TYPE_NONE;
    int             m_trackID = -1;
    size_t          m_streamIndex = 0;
};
using HJStreamInfoList = std::list<HJStreamInfo::Ptr>;
using HJStreamInfoMap = std::unordered_map<std::string, HJStreamInfo::Ptr>;
#define HJMediaType2String(v)  HJ::HJStreamInfo::type2String(v)

class HJSubtitleInfo : public HJStreamInfo
{
public:
    using Ptr = std::shared_ptr<HJSubtitleInfo>;
    HJSubtitleInfo() {
        m_type = HJMEDIA_TYPE_SUBTITLE;
    }
    virtual void clone(const HJStreamInfo::Ptr& other) override {
        HJStreamInfo::clone(other);
        //
        HJSubtitleInfo::Ptr otherInfo = std::dynamic_pointer_cast<HJSubtitleInfo>(other);
        m_txtFmt = otherInfo->m_txtFmt;
    }
    virtual HJStreamInfo::Ptr dup() override {
        HJSubtitleInfo::Ptr info = std::make_shared<HJSubtitleInfo>();
        info->clone(sharedFrom(this));
        return info;
    }
public:
    int     m_txtFmt = 0;
};

class HJChannelLayout : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJChannelLayout>;
    HJChannelLayout(const int channel);
    HJChannelLayout(const AVChannelLayout* avchLayout);
    virtual ~HJChannelLayout();

    const AVChannelLayout* getAVCHLayout() const {
        return m_avchLayout;
    }
    void setAVCHLayout(AVChannelLayout* avchLayout) {
        m_avchLayout = avchLayout;
    }
private:
    AVChannelLayout* m_avchLayout;
};

#define HJ_CHANNEL_DEFAULT         2
#define HJ_SAMPLE_RATE_DEFAULT     44100
#define HJ_SAMPLE_FORMAT_DEFAULT   8               //AV_SAMPLE_FMT_FLTP
#define HJ_FRAME_SAMPLES_DEFAULT   1024

class HJAudioInfo : public HJStreamInfo
{
public:
    using Ptr = std::shared_ptr<HJAudioInfo>;
    HJAudioInfo();
    virtual ~HJAudioInfo();
    
    virtual void cloneAudioInfo(const HJAudioInfo::Ptr& other);
    virtual void clone(const HJStreamInfo::Ptr& other) override {
        HJStreamInfo::clone(other);
        //
        HJAudioInfo::Ptr ainfo = std::dynamic_pointer_cast<HJAudioInfo>(other);
        cloneAudioInfo(ainfo);
    }
    virtual HJStreamInfo::Ptr dup() override {
        HJAudioInfo::Ptr info = std::make_shared<HJAudioInfo>();
        info->clone(sharedFrom(this));
        return info;
    }
    //
    void setDefaultChannels() {
        setChannels(HJ_CHANNEL_DEFAULT);
    }
    void setChannels(int channels, const AVChannelLayout* avchLayout = NULL);
    void setChannels(const AVChannelLayout* layout);
    int getChannels() const 
    {
        return m_channels;
    }
    AVChannelLayout* getAVChannelLayout();
    HJChannelLayout::Ptr getChannelLayout() {
		auto it = find(STORE_KEY_HJChannelLayout);
		if (it != end()) {
			return std::any_cast<HJChannelLayout::Ptr>(it->second);
		}
		return nullptr;
    }
    void setChannelLayout(const HJChannelLayout::Ptr& chLayout) {
        if (chLayout) {
            (*this)[STORE_KEY_HJChannelLayout] = chLayout;
        }
    }
    HJChannelLayout::Ptr getDefaultChannelLayout() {
        return std::make_shared<HJChannelLayout>(m_channels);
    }
    void setDefaultSampleRate() {
        m_samplesRate = HJ_SAMPLE_RATE_DEFAULT;
    }
    void setSampleRate(int sampleRate) {
        m_samplesRate = sampleRate;
    }
    const int getSampleRate() const {
        return m_samplesRate;
    }
    void setSampleCnt(int sampleCnt) {
        m_sampleCnt = sampleCnt;
    }
    const int getSampleCnt() {
        return m_sampleCnt;
    }
    bool operator!=(const HJAudioInfo& other) const;
    
    virtual const std::string formatInfo() override;
public:
    static const std::string STORE_KEY_HJChannelLayout;    //HJChannelLayout
    //
    static HJAudioInfo::Ptr makeDefaultInfo() {
        HJAudioInfo::Ptr ainfo = std::make_shared<HJAudioInfo>();
        ainfo->setDefaultChannels();
        ainfo->setDefaultSampleRate();
        return ainfo;
    }
public:
    int         m_channels = HJ_CHANNEL_DEFAULT;
    int         m_bytesPerSample = 4;
    int         m_sampleFmt = 8;                //AV_SAMPLE_FMT_S16 1; AV_SAMPLE_FMT_FLT 3  //AV_SAMPLE_FMT_FLTP 8
    int         m_samplesRate = HJ_SAMPLE_RATE_DEFAULT;
    int         m_blockAlign = 0;
    int         m_samplesPerFrame = HJ_FRAME_SAMPLES_DEFAULT;
//    uint64_t    m_channelLayout = 0;
    int         m_sampleLayout = 0;
    int         m_sampleCnt = HJ_FRAME_SAMPLES_DEFAULT;
};

class HJVideoInfo : public HJStreamInfo
{
public:
    using Ptr = std::shared_ptr<HJVideoInfo>;
    HJVideoInfo();
    HJVideoInfo(int width, int height);
    virtual ~HJVideoInfo();
    
    virtual void cloneVideoInfo(const HJVideoInfo::Ptr& other);
    virtual void clone(const HJStreamInfo::Ptr& other) override {
        HJStreamInfo::clone(other);
        //
        HJVideoInfo::Ptr vinfo = std::dynamic_pointer_cast<HJVideoInfo>(other);
        cloneVideoInfo(vinfo);
    }
    virtual HJStreamInfo::Ptr dup() override {
        HJVideoInfo::Ptr info = std::make_shared<HJVideoInfo>();
        info->clone(sharedFrom(this));
        return info;
    }
    HJHWFrameCtx::Ptr getHWFramesCtx();
    HJHND getMediaCodecSurface();
    
    virtual const std::string formatInfo() override;
public:
    static const std::string STORE_KEY_HWFramesCtx;         //hw_frames_ctx
    static const std::string STORE_KEY_MediaCodecSurface;   //mediacodec_surface
public:
    int         m_width = 0;
    int         m_height = 0;
    int         m_stride[4] = {0};
    int         m_pixFmt = 0;   //AV_PIX_FMT_YUV420P
    HJRational m_aspectRatio = {1, 1};
    int         m_scaleMode = 0;
    int         m_frameRate = HJ_FRAMERATE_DEFALUT;
    int         m_gopSize = HJ_FRAMERATE_DEFALUT;
    int         m_maxBFrames = 0;
    int         m_lookahead = HJ_FRAMERATE_DEFALUT;
    float       m_quality = 0.8f;
    float       m_rotate = 0.0f;
    //
    int         m_colorRange;
    int         m_colorPrimaries;
    int         m_colorTrc;
    int         m_colorSpace;
    int         m_chromaLocation;
};

//***********************************************************************************//
class HJMediaInfo : public HJKeyStorage, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJMediaInfo>;
    HJMediaInfo(const int mediaType = HJMEDIA_TYPE_NONE, const std::string& url = "");
    virtual ~HJMediaInfo();
    
    void setMediaUrl(const HJMediaUrl::Ptr& url) {
        m_url = url;
    }
    const HJMediaUrl::Ptr& getMediaUrl() {
        return m_url;
    }
    const HJStreamInfoList& getStreamInfos() const {
        return m_streamInfos;
    }
    void setMediaTypes(const int mediaTypes) {
        m_mediaTypes = mediaTypes;
    }
    const int getMediaTypes() const {
        return m_mediaTypes;
    }
    const int64_t getStartTime() const {
        return m_startTime;
    }
    void setStartTime(const int64_t time) {
        m_startTime = time;
    }
    const int64_t getDuration() const {
        return m_duration;
    }
    void setDuration(const int64_t duration) {
        m_duration = duration;
    }
    void setTimeRange(const HJTimeRange timeRange) {
        m_timeRange = timeRange;
    }
    const HJTimeRange getTimeRange() const {
        return m_timeRange;
    }
    void addStreamInfo(const HJStreamInfo::Ptr& info) {
        if(info->getID() <= 0) {
            info->setID(m_streamInfos.size());
        }
        m_mediaTypes |= info->getType();
        m_streamInfos.push_back(info);
    }
    const HJStreamInfo::Ptr getStreamInfo(const HJMediaType type) {
        for (auto &info : m_streamInfos) {
            if(type == info->getType()) {
                return info;
            }
        }
        return nullptr;
    }
    int forEachInfo(const std::function<int(const HJStreamInfo::Ptr &)>& cb);

    void clearStreamInfos() {
        m_streamInfos.clear();
        m_mediaTypes = HJMediaType::HJMEDIA_TYPE_NONE;
    }
    
    const HJVideoInfo::Ptr getVideoInfo() {
        return std::dynamic_pointer_cast<HJVideoInfo>(getStreamInfo(HJMEDIA_TYPE_VIDEO));
    }
    const HJAudioInfo::Ptr getAudioInfo() {
        return std::dynamic_pointer_cast<HJAudioInfo>(getStreamInfo(HJMEDIA_TYPE_AUDIO));
    }
    
    //
    virtual void clone(const HJMediaInfo::Ptr& other)
    {
        m_mediaTypes = other->m_mediaTypes;
        m_astIdx = other->m_astIdx;
        m_vstIdx = other->m_vstIdx;
        m_sstIdx = other->m_sstIdx;
        m_startTime = other->m_startTime;
        m_duration = other->m_duration;
        m_timeRange = other->m_timeRange;
        m_url = other->m_url;
    }
    virtual HJMediaInfo::Ptr dup()
    {
        HJMediaInfo::Ptr info = std::make_shared<HJMediaInfo>();
        info->clone(sharedFrom(this));
        for (auto &it : m_streamInfos) {
            info->addStreamInfo(it->dup());
        }
        return info;
    }
    //
    void addSubMInfo(const HJMediaInfo::Ptr& minfo) {
        m_subMInfos.emplace_back(minfo);
    }
    auto& getSubMInfos() {
        return m_subMInfos;
    }
    const HJMediaInfo::Ptr getDefaultSubMInfo() {
        if (m_subMInfos.size() > 0) {
            return m_subMInfos[0];
        }
        return nullptr;
    }
    //
    virtual const std::string formatInfo();
public:
    static const std::string KEY_WORLDS;
public:
    int                 m_mediaTypes{ HJMEDIA_TYPE_NONE };
    int                 m_astIdx = -1;
    int                 m_vstIdx = -1;
    int                 m_sstIdx = -1;
    int64_t             m_startTime = 0;        //ms
    int64_t             m_duration = 0;         //ms
    HJTimeRange        m_timeRange = {HJUtilTime::HJTimeZero, HJUtilTime::HJTimeZero};
    //streams
    HJStreamInfoList   m_streamInfos;
    HJMediaUrl::Ptr     m_url = nullptr;
    //
    std::vector<HJMediaInfo::Ptr>   m_subMInfos;
};
using HJMediaInfoVector = std::vector<HJMediaInfo::Ptr>;

//***********************************************************************************//
class HJSeekInfo : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJSeekInfo>;
    HJSeekInfo(const int64_t pos = 0);
    
    void setPos(int64_t pos) {
        m_pos = pos;
    }
    const int64_t getPos() const {
        return m_pos;
    }
    void setTime(int64_t time) {
        m_time = time;
    }
    const int64_t getTime() const {
        return m_time;
    }
public:
    static const std::string KEY_WORLDS;
private:
    int64_t        m_pos = 0;
    int64_t        m_time = 0;
};

//***********************************************************************************//
typedef struct HJRoiInfo {
    float quant_offset;
    int x, y, w, h;
} HJRoiInfo;
using HJRoiInfoVector = std::vector<HJRoiInfo>;
using HJRoiInfoVectorPtr = std::shared_ptr<HJRoiInfoVector>;

typedef enum HJRoiEncodeCbFlag { 
    HJ_ROI_CB_FLAG_NONE = 0x00,
    HJ_ROI_CB_FLAG_TRY_ACQUIRE,
    HJ_ROI_CB_FLAG_CLOSE,
} HJRoiCbFlag;
using HJRoiEncodeCb = std::function<void (HJRoiInfoVectorPtr &roiInfo, int i_nFlag)>;

//***********************************************************************************//
class HJProgressInfo : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJProgressInfo>;
    HJProgressInfo(const int64_t pos = 0, const int64_t duration = 0) 
        : m_pos(pos)
        , m_duration(duration){ }

    void setPos(int64_t pos) {
        m_pos = pos;
    }
    const int64_t getPos() const {
        return m_pos;
    }
    void setDuration(int64_t duration) {
        m_duration = duration;
    }
    const int64_t getDuration() const {
        return m_duration;
    }
public:
    static const std::string KEY_WORLDS;
protected:
    int64_t        m_pos = 0;
    int64_t        m_duration = HJ_INT16_MAX;
};

NS_HJ_END
