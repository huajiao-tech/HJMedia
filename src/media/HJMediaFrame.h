//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaInfo.h"
#include "HJTicker.h"

typedef struct AVPacket AVPacket;
typedef struct AVFrame AVFrame;
typedef struct SwsContext SwsContext;

NS_HJ_BEGIN
#define HJ_DEFAULT_STORAGE_CAPACITY    5
#define HJ_ADEC_STOREAGE_CAPACITY      50
#define HJ_VDEC_STOREAGE_CAPACITY      15
#define HJ_AENC_STOREAGE_CAPACITY      100
#define HJ_VENC_STOREAGE_CAPACITY      3
#define HJ_AREND_STOREAGE_CAPACITY     10
#define HJ_VREND_STOREAGE_CAPACITY     3
//***********************************************************************************//
#define HJ_FRAME_FLAG_RESTRICT_DELAY       0x0001

typedef enum HJFrameType
{
    HJFRAME_NONE   = 0x00,
    HJFRAME_NORMAL = 0x01,
    HJFRAME_KEY    = 0x02,
    HJFRAME_EOF    = 0x04,
    HJFRAME_NULL   = 0x08,
    HJFRAME_RESERVED = 0x00FF,
    HJFRAME_FLUSH    = 0x0100,
} HJFrameType;
#define HJFRAME_MAINTYPE(v)    (v & HJFRAME_RESERVED)
#define HJFRAME_OPTSTYPE(v)    (v & ~HJFRAME_RESERVED)

/**
 * video encode stream frame type
 */
typedef enum HJVESFrameType
{
    HJ_VESF_NONE    = -1,
    HJ_VESF_IDR,
    HJ_VESF_I,
    HJ_VESF_P,
    HJ_VESF_BRef,
    HJ_VESF_B
} HJVESFrameType;

class HJAVFrame : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAVFrame>;
	HJAVFrame();
    HJAVFrame(AVFrame* avf);
	virtual ~HJAVFrame();

    AVFrame* getAVFrame() const {
        return m_avf;
    }
    const std::string getKey();

    uint8_t* getPixelBuffer();

    void setTimeBase(const HJTimeBase& timeBase);
    void setTimeStamp(const int64_t pts, const int64_t dts, const HJTimeBase tb = HJ_TIMEBASE_MS);
    //
    virtual HJAVFrame::Ptr dup();
public:
    uint8_t*  m_datas[4] = {NULL, };
private:
    AVFrame*  m_avf = NULL;
};
using HJAVFrameVector = std::vector<HJAVFrame::Ptr>;

class HJAVPacket : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAVPacket>;
	HJAVPacket();
    HJAVPacket(AVPacket* pkt);
    virtual ~HJAVPacket();

    AVPacket* getAVPacket() const {
        return m_pkt;
    }
    void setTimeBase(const HJTimeBase& timeBase);
    void setTimeStamp(const int64_t pts, const int64_t dts, const HJTimeBase tb = HJ_TIMEBASE_MS);
    //
    virtual HJAVPacket::Ptr dup();
private:
    AVPacket*  m_pkt = NULL;
};
using HJAVPacketVector = std::vector<HJAVPacket::Ptr>;

class HJAVBuffer : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAVBuffer>;
    HJAVBuffer(const HJAVBuffer::Ptr& other);
    HJAVBuffer(uint8_t* data, const size_t size);
    HJAVBuffer(size_t capacity);
    virtual ~HJAVBuffer();

    uint8_t* getData() {
        return m_data;
    }
    const size_t getSize() {
        return m_size;
    }
    uint8_t* removeData() {
        uint8_t* data = m_data;
        m_data = NULL;
        m_size = 0;
        return data;
    }
    void appendData(const uint8_t* data, size_t size);
    void write(const uint8_t* data, size_t size) {
        if (data && size > 0) {
            appendData(data, size);
        }
    }
public:
    static const std::string KEY_WORLDS;
private:
    size_t   m_capacity{ 0 };
    uint8_t* m_data{ NULL };
    size_t   m_size{ 0 };
};

class HJMediaFrame : public HJKeyStorage, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJMediaFrame>;
    //
    HJMediaFrame(const HJStreamInfo::Ptr& info = nullptr);
    virtual ~HJMediaFrame();

    int setData(uint8_t* data, size_t size);
    int appendData(uint8_t* data, size_t size);
    const HJBuffer::Ptr& getBuffer() const {
        return m_Buffer;;
    }
    void setBuffer(const HJBuffer::Ptr& buffer) {
        m_Buffer = buffer;
    }
    
    const HJMediaType getType() const {
        if (m_info) {
            return m_info->getType();
        }
        return HJMEDIA_TYPE_NONE;
    }
    bool isVideo() {
        return (HJMEDIA_TYPE_VIDEO == getType());
    }
    bool isAudio() {
        return (HJMEDIA_TYPE_AUDIO == getType());
    }
    const size_t getIndex() const {
        return m_index;
    }
    const void setIndex(const size_t index) {
        m_index = index;
    }
    const void setPTS(const int64_t pts) {
        m_pts = pts;
    }
    const int64_t getPTS() const {
        return m_pts;
    }
    const void setDTS(const int64_t dts) {
        m_dts = dts;
    }
    const int64_t getDTS() const {
        return m_dts;
    }
    const int64_t getDTSUS();
    const int64_t getPTSUS();
    void setPTSDTS(const int64_t pts, const int64_t dts, const HJTimeBase tb = HJ_TIMEBASE_MS);
    void setAVTimeBase(const HJTimeBase& timeBase);
    //
    void setDuration(const int64_t duration, const HJRational timeBase);
    void setDuration(const int64_t duration) {
        m_duration = duration;
    }
    const int64_t getDuration() const {
        return m_duration;
    }
	void setTimeBase(const HJRational timeBase) {
		m_timeBase = timeBase;
	}
	void setTimeBase(const int num, const int den) {
		m_timeBase = { num, den };
	}
	const HJRational getTimeBase() const {
		return m_timeBase;
	}
    void setMTS(const int64_t pts, const int64_t dts, const int64_t duration, const HJTimeBase tb = HJ_TIMEBASE_US);
    void setMTS(const HJMediaTime& mts) {
        m_mts = mts;
    }
    auto& getMTS() {
        return m_mts;
    }
    void setExtraTS(const int64_t exts) {
        m_extraTS = exts;
    }
    const int64_t getExtraTS() const {
        return m_extraTS;
    }
    void setSpeed(const float speed) {
        m_speed = speed;
    }
    const float getSpeed() const {
        return m_speed;
    }
    void setFrameType(const int type) {
        m_frameType = type;
    }
    const int getFrameType() const {
        return HJFRAME_MAINTYPE(m_frameType);
    }
    std::string getFrameTypeStr() {
        return HJMediaFrame::frameType2Name(m_frameType);
    }
    void setVESFrameType(const HJVESFrameType type) {
        m_vesfType = type;
    }
    const HJVESFrameType getVESFrameType() const {
        return m_vesfType;
    }
    std::string getVESFrameTypeStr() {
        return HJMediaFrame::vesfType2Name(m_vesfType);
    }
    const bool isKeyFrame() const {
        return (HJFRAME_KEY & m_frameType);
    }
    const bool isEofFrame() const {
        return (HJFRAME_EOF & m_frameType);
    }
    const bool isFlushFrame() const {
        return (HJFRAME_FLUSH & m_frameType);
    }
    void setFlush() {
        m_frameType = m_frameType | HJFRAME_FLUSH;
    }
    const bool isValidFrame() const {
        return ((HJFRAME_NORMAL & m_frameType) ||
                (HJFRAME_KEY & m_frameType) ||
                (HJFRAME_FLUSH & m_frameType));
    }
    template<typename T>
    const T getStreamInfo() {
        auto it = find(STORE_KEY_STREAMINFO);
        if (it != end()) {
            return std::any_cast<T>(it->second);
        }
        return nullptr;
    }
    void setInfo(const HJStreamInfo::Ptr& info) {
        m_info = info;
    }
    const auto getInfo() const {
        return m_info;
    }
    void cloneInfo(const HJStreamInfo::Ptr& info)
    {
        if (!m_info || !info || info->getType() != m_info->getType()) {
            return;
        }
        m_info->clone(info);
    }
    const HJMediaInfo::Ptr getMediaInfo() {
        auto it = find(STORE_KEY_MEDIAINFO);
        if (it != end()) {
            return std::any_cast<HJMediaInfo::Ptr>(it->second);
        }
        return nullptr;
    }
    const HJAudioInfo::Ptr getAudioInfo() {
        if (HJ_ISTYPE(*m_info, HJAudioInfo)) {
            return std::dynamic_pointer_cast<HJAudioInfo>(m_info);
        }
        return nullptr;
    }
    const HJVideoInfo::Ptr getVideoInfo() {
		if (HJ_ISTYPE(*m_info, HJVideoInfo)) {
			return std::dynamic_pointer_cast<HJVideoInfo>(m_info);
		}
        return nullptr;
    }
    const HJDataType getDataType() {
        return m_info ? m_info->getDataType() : HJDATA_TYPE_NONE;
    }
    const std::string getTrackKey() {
        return HJMediaType2String(getType()) + HJ2STR(m_id);
    }
    //
    void setMPacket(AVPacket* pkt);
    void setMPacket(const HJAVPacket::Ptr& mpkt);
    HJAVPacket::Ptr getMPacket();
    AVPacket* getAVPacket();

    void setMFrame(AVFrame* avf);
    void setMFrame(const HJAVFrame::Ptr& mavf);
    HJAVFrame::Ptr getMFrame();
    AVFrame* getAVFrame();

    void setAuxMFrame(const HJAVFrame::Ptr& mavf);
    HJAVFrameVector getAuxMFrames();
    HJAVFrameVector getALLMFrames();

    HJHND makeAVFrame();
    uint8_t* getPixelBuffer();
    //
    const std::string formatInfo(bool trace = false);
    void addTrajectory(const HJTrajectory::Ptr& trajectory);
    void dupTracker(const HJMediaFrame::Ptr& other);
    const HJTracker::Ptr& getTracker() {
        return m_tracker;
    }
    void setTracker(const HJTracker::Ptr& tracker) {
        m_tracker = tracker;
    }
    virtual void clone(const HJMediaFrame::Ptr& other);
    virtual HJMediaFrame::Ptr dup();
    virtual HJMediaFrame::Ptr deepDup();
public:
    static HJMediaFrame::Ptr makeVideoFrame(const HJVideoInfo::Ptr& info = nullptr);
    static HJMediaFrame::Ptr makeAudioFrame(const HJAudioInfo::Ptr& info = nullptr);
    static HJMediaFrame::Ptr makeSilenceAudioFrame(const HJAudioInfo::Ptr info = nullptr);
    static HJMediaFrame::Ptr makeSilenceAudioFrame(int channel, int sampleRate, int sampleFmt, int nbSamples = HJ_FRAME_SAMPLES_DEFAULT);
    static HJMediaFrame::Ptr makeDefaultSilenceAudioFrame();
    static HJMediaFrame::Ptr makeEofFrame(const HJMediaType type = HJMEDIA_TYPE_NONE);
    static HJMediaFrame::Ptr makeFlushFrame(const int streamIndex, const HJMediaInfo::Ptr info = nullptr, int flag = 0);
    static HJMediaFrame::Ptr makeFlushFrame(const int streamIndex, const HJStreamInfo::Ptr info, int flag = 0);

    static int makeAVCodecParam(const HJStreamInfo::Ptr &i_info, HJBuffer::Ptr i_extradata);
    static HJCodecParameters::Ptr makeHJAVCodecParam(const HJStreamInfo::Ptr &i_info, HJBuffer::Ptr i_extradata);
    static HJMediaFrame::Ptr makeMediaFrameAsAVPacket(const HJStreamInfo::Ptr &i_info, uint8_t *i_data, int i_size, bool i_bKey, int64_t i_pts, int64_t i_dts, const HJRational& i_ratio);
    static HJMediaFrame::Ptr makeMediaFrameAsAVFrame(const HJStreamInfo::Ptr &i_info, uint8_t *i_data, int i_size, bool i_bKey, int64_t i_pts, const HJRational& i_ratio);
    static int getDataFromAVFrame(const HJMediaFrame::Ptr &i_frame, uint8_t *&o_data, int &o_size);
    static void setCodecIdAAC(int & o_codecId);
    static bool isCodecMatchAAC(int i_codecId);
    //
    static const std::string KEY_WORLDS;
    static const std::string STORE_KEY_HJAVPACKET;
    static const std::string STORE_KEY_HJAVFRAME;
    static const std::string STORE_KEY_MEDIAINFO;
    static const std::string STORE_KEY_STREAMINFO;
    static const std::string STORE_KEY_SEIINFO;
    static const std::string STORE_KEY_ROIINFO;
    static const std::string STORE_KEY_VID_BITRATE;     //video bitrate
    static const std::string STORE_KEY_EXTRADATAS;
    //static const std::string makeAVFrameStoreKey(int width, int height, int fmt);
    //
    static const std::string frameType2Name(const int& type);
    static const std::string vesfType2Name(const HJVESFrameType& type);
public:
    size_t              m_index = 0;      //order count, frame index
    int64_t             m_pts = HJ_NOTS_VALUE; //-1;
    int64_t             m_dts = HJ_NOTS_VALUE; //-1;
    int64_t             m_duration = 0;
    HJRational          m_timeBase = HJ_TIMEBASE_MS;   //{ 0, 1 };
    HJMediaTime         m_mts = HJMediaTime::HJMediaTimeZero;
    int64_t             m_extraTS = HJ_NOTS_VALUE;     //extra timestamp, capture time, etc.
    float               m_speed = 1.0f;
    int                 m_frameType = HJFRAME_NORMAL;
    HJVESFrameType      m_vesfType = HJ_VESF_NONE;
    HJBuffer::Ptr       m_Buffer = nullptr;
    HJStreamInfo::Ptr   m_info = nullptr;
    // _GS_
    int                 m_trackIndex = 0;
    int                 m_streamIndex = 0;
    int                 m_flag = 0;
    HJTracker::Ptr      m_tracker = nullptr;
};
using HJMediaFrameList = HJList<HJMediaFrame::Ptr>;
using HJMediaFrameMap = std::multimap<int64_t, HJMediaFrame::Ptr>;
using HJMediaFrameListener = std::function<int(const HJMediaFrame::Ptr)>;

//***********************************************************************************//
class HJMediaStorage : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJMediaStorage>;
    HJMediaStorage(size_t capacity = HJ_DEFAULT_STORAGE_CAPACITY, const std::string& name = "");
    virtual ~HJMediaStorage();
    
    int push(HJMediaFrame::Ptr&& frame);
    int pushFresh(HJMediaFrame::Ptr&& frame);
    HJMediaFrame::Ptr pop();
    HJMediaFrame::Ptr next();
    HJMediaFrame::Ptr last();
    bool isFull();
    bool hasEofFrame() {
        return m_hasEofFrame;
    }
    void clear();
    size_t size();
    const size_t getTotalInCnt();
    const size_t getTotalOutCnt();
    int64_t getTopDTS();
    int64_t getTopPTS();
    int64_t getLastDTS();
    int64_t getDTSDuration();
    void setNextDTS(const int64_t dts) {
        m_nextDTS = dts;
    }
    const int64_t getNextDTS() {
        return m_nextDTS;
    }
    const int64_t getCurDTS() {
        return m_curDTS;
    }
    void setMTS(const HJUtilTime mts) {
        m_mts = mts;
    }
    HJUtilTime& getMTS() {
        return m_mts;
    }
    int64_t getDuration();
    bool isEOF();

    /**
    * 
    */
    enum class StoreType : uint8_t
    {
        NONE = 0,
        TIME,
    };
    void setStoreType(const StoreType type) {
        m_storeType = type;
    }
    const StoreType getStoreType() const {
        return m_storeType;
    }
    void setCapacity(const size_t capacity) {
        HJ_AUTO_LOCK(m_mutex);
        m_capacity = capacity;
    }
    const size_t getCapacity() const {
        return m_capacity;
    }
    void setTimeCapacity(const int64_t capacity) {
        HJ_AUTO_LOCK(m_mutex);
        m_storeType = StoreType::TIME;
        m_timeCapacity = capacity;
        m_capacity = 2 * capacity / 20;
    }
    const int64_t getTimeCapacity() const {
        return m_timeCapacity;
    }
    virtual void setMediaType(const HJMediaType& type) {
        m_mediaType = type;
    }
    const HJMediaType& getMediaType() const {
        return m_mediaType;
    }
private:
    void addDuration(const HJMediaFrame::Ptr& frame) {
        auto it = m_durations.find(frame->m_frameType);
        if (it != m_durations.end()) {
            m_durations[frame->m_frameType] = it->second + frame->m_duration;
        } else {
            m_durations[frame->m_frameType] = frame->m_duration;
        }
    }
    void subDuration(const HJMediaFrame::Ptr& frame) {
        auto it = m_durations.find(frame->m_frameType);
        if (it != m_durations.end()) {
            m_durations[frame->m_frameType] = it->second - frame->m_duration;
        }
    }
    int64_t getDuration(const HJMediaType& type) {
        auto it = m_durations.find(type);
        if (it != m_durations.end()) {
            return it->second;
        }
        return 0;
    }
    int64_t getMinDuration() {
        int64_t minDuration = HJ_INT64_MAX;
        for (auto it : m_durations) {
            if (it.second < minDuration) {
                minDuration = it.second;
            }
        }
        return (minDuration < HJ_INT64_MAX) ? minDuration : 0;
    }
protected:
    size_t                           m_capacity = HJ_DEFAULT_STORAGE_CAPACITY;
    int64_t                          m_timeCapacity = HJ_SEC_TO_MS(5);
    StoreType                        m_storeType = StoreType::NONE;
    HJMediaFrameList                 m_mediaFrames;
    std::recursive_mutex             m_mutex;
    size_t                           m_inCnt = 0;
    size_t                           m_outCnt = 0;
    std::atomic_int64_t              m_curDTS = { HJ_INT64_MIN };
    std::atomic_int64_t              m_nextDTS = { HJ_INT64_MIN };
    HJMediaType                      m_mediaType = HJMEDIA_TYPE_NONE;
    std::unordered_map<int, int64_t> m_durations;
    bool                             m_hasEofFrame = false;
    //
    HJUtilTime                       m_mts = HJUtilTime::HJTimeNOTS;
};
using HJMediaFrameCache = HJMediaStorage;
using HJMediaStorageMap = std::unordered_map<std::string, HJMediaStorage::Ptr>;
using HJMediaFrameCachesMap = std::unordered_map<std::string, HJMediaFrameCache::Ptr>;

//***********************************************************************************//
typedef int (*SLIOCallback)(void*);

typedef struct MTIOInterruptCB {
	MTIOInterruptCB(SLIOCallback cb = nullptr, void* the = nullptr)
		: callback(cb)
		, opaque(the) { }

	MTIOInterruptCB(MTIOInterruptCB* ioCallback)
		: callback(ioCallback->callback)
		, opaque(ioCallback->opaque) { }

	SLIOCallback callback;
	void* opaque;

	MTIOInterruptCB& operator=(const MTIOInterruptCB& _Right) {
		if (this == &_Right) return *this;
		callback = _Right.callback;
		opaque = _Right.opaque;
		return *this;
	}
} MTIOInterruptCB;

//***********************************************************************************//
class HJAFrameBrief
{
public:
    HJAFrameBrief(HJUtilTime pts, int nbSamples)
        : m_pts(std::move(pts))
        , m_nbSamples(nbSamples)
        , m_lastNBSamples(nbSamples) {}
public:
    HJUtilTime     m_pts = HJUtilTime::HJTimeZero;
    int         m_nbSamples = 0;
    int         m_lastNBSamples = 0;
    float       m_speed = 1.0f;
};
using HJAFrameBriefList = HJList<HJAFrameBrief>;

NS_HJ_END
