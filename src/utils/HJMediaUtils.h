//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <utility>
#include <cstdint>
#include "HJCommons.h"
#include "HJBuffer.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJAPPWillState {
    HJAPPWillResignActive,     //UIApplicationWillResignActiveNotification
    HJAPPDidEnterBackground,   //UIApplicationDidEnterBackgroundNotification
    HJAPPWillEnterForeground,  //UIApplicationWillEnterForegroundNotification
    HJAPPDidBecomeActive,      //UIApplicationDidBecomeActiveNotification
    HJAPPWillTerminate,        //UIApplicationWillTerminateNotification
} HJAPPWillState;

typedef enum HJDeviceType {
    HJDEVICE_TYPE_NONE,          //software
    HJDEVICE_TYPE_VDPAU,
    HJDEVICE_TYPE_CUDA,
    HJDEVICE_TYPE_VAAPI,
    HJDEVICE_TYPE_DXVA2,
    HJDEVICE_TYPE_QSV,
    HJDEVICE_TYPE_VIDEOTOOLBOX,
    HJDEVICE_TYPE_D3D11VA,
    HJDEVICE_TYPE_DRM,
    HJDEVICE_TYPE_OPENCL,
    HJDEVICE_TYPE_MEDIACODEC,
    HJDEVICE_TYPE_VULKAN,
    HJDEVICE_TYPE_TSCSDK,
    HJDEVICE_TYPE_OHCODEC,   //harmony
} HJDeviceType;

typedef enum HJCaptureType {
    HJCAPTURE_TYPE_NONE,          //software
    HJCAPTURE_TYPE_OH,
    
    
} HJCaptureType;

typedef enum HJCodecID {
    HJ_CODEC_ID_NONE,
    HJ_CODEC_ID_H264,
    HJ_CODEC_ID_H265,
    HJ_CODEC_ID_AAC,
    HJ_CODEC_ID_OPUS,
} HJCodecID;

typedef enum HJSampleFormat {
    HJ_SAMPLE_FMT_NONE = -1,
    HJ_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    HJ_SAMPLE_FMT_S16,         ///< signed 16 bits
    HJ_SAMPLE_FMT_S32,         ///< signed 32 bits
    HJ_SAMPLE_FMT_FLT,         ///< float
    HJ_SAMPLE_FMT_DBL,         ///< double

    HJ_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    HJ_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    HJ_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    HJ_SAMPLE_FMT_FLTP,        ///< float, planar
    HJ_SAMPLE_FMT_DBLP,        ///< double, planar
    HJ_SAMPLE_FMT_S64,         ///< signed 64 bits
    HJ_SAMPLE_FMT_S64P,        ///< signed 64 bits, planar

    HJ_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
} HJSampleFormat;

typedef enum HJVCropMode
{
    HJ_VCROP_FIT = 0,
    HJ_VCROP_CLIP,
    HJ_VCROP_FILL,
    HJ_VCROP_ORIGINAL,
} HJVCropMode;

typedef struct HJRational {
	int num;    ///< Numerator
	int den;    ///< Denominator
    
    bool isMatch(const HJRational& i_rational)
    {
        return ((num == i_rational.num) && (den == i_rational.den));
    }
    
} HJRational;
static const HJRational HJ_RATIONAL_DEFAULT = { 0, 1 };
static const HJRational HJ_RATIONAL_ONE = { 1, 1 };
static const HJRational HJRate15 = { 15, 1000 };
static const HJRational HJRate24 = { 24, 1000 };
static const HJRational HJRate30 = { 30, 1000 };
//
typedef struct HJRational HJTimeBase;
static const HJTimeBase HJ_TIMEBASE_MS = { 1, 1000 };         //ms
static const HJTimeBase HJ_TIMEBASE_US = { 1, 1000000 };      //us
static const HJTimeBase HJ_TIMEBASE_9W = { 1, 90000 };        //90000
#define HJ_FRAME_DURATION(tbase, rate)     (tbase.den * rate.den / (tbase.num * (int64_t)rate.num))

//
template<typename T>
struct HJPostion
{
    T x;
    T y;
};
using HJPostioni = HJPostion<int>;
using HJPostionf = HJPostion<float>;
using HJPostiond = HJPostion<double>;
static const HJPostioni HJ_POS_ZERO = { 0, 0 };

template<typename T>
struct HJSize
{
    T w;
    T h;
};
using HJSizei = HJSize<int>;
using HJSizef = HJSize<float>;
using HJSized = HJSize<double>;
static const HJSizei HJ_SIZE_ZERO = {0, 0};
static const HJSizei HJ_SIZE_720P = { 1280, 720 };
static const HJSizei HJ_SIZE_1080P = { 1920, 1080 };
static const HJSizei HJ_SIZE_2K = { 2560, 1440 };
static const HJSizei HJ_SIZE_4K = { 3840, 2160 };
static const HJSizei HJ_SIZE_8K = { 7680, 4320 };

template<typename T>
struct HJPoint
{
    T x;
    T y;
};
using HJPointi = HJPoint<int>;
using HJPointf = HJPoint<float>;
using HJPointd = HJPoint<double>;
static const HJPointf HJ_POINT_ZERO = { 0.0f, 0.0f };

template<typename T>
struct HJRect
{
    T x;
    T y;
    T w;
    T h;
};
using HJRecti = HJRect<int>;
using HJRectf = HJRect<float>;
static const HJRecti HJ_RECT_ZERO = { 0, 0, 0, 0 };
static const HJRectf HJ_RECT_DEFAULT = {0.0f, 0.0f, 1.0f, 1.0f};

template<typename T>
union HJVec4
{
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    T v[4];
};
using HJVec4c = HJVec4<uint8_t>;
using HJVec4i = HJVec4<int>;
using HJVec4f = HJVec4<float>;
using HJColor = HJVec4<float>;
static const HJColor HJ_COLOR_BLACK = { 0.0, 0.0, 0.0, 0.0 };
static const HJColor HJ_COLOR_RED   = { 1.0, 0.0, 0.0, 1.0 };
static const HJColor HJ_COLOR_GREEN = { 0.0, 1.0, 0.0, 1.0 };
static const HJColor HJ_COLOR_BLUE  = { 0.0, 0.0, 1.0, 1.0 };
static const HJColor HJ_COLOR_DEFAULT = { 0.48235f, 0.40784f, 0.93333f, 0.7f }; //123, 104, 238

template<typename T>
struct HJRange
{
    T begin;
    T end;

    T size() const { return end - begin + 1; }
    bool empty() const { return begin == end; }
    bool contains(const T& value) const { return value >= begin && value < end; }
    bool intersects(const HJRange& other) const { return other.begin < end && other.end > begin; }
    bool operator==(const HJRange& other) const { return begin == other.begin && end == other.end; }
    bool operator!=(const HJRange& other) const { return begin!= other.begin || end!= other.end; }
    HJRange& operator+=(const T& value) { begin += value; end += value; return *this; }
    HJRange& operator-=(const T& value) { begin -= value; end -= value; return *this; }
};
using HJRangei     = HJRange<int32_t>;
using HJRange64i   = HJRange<int64_t>;
using HJRangef     = HJRange<float>;
static const HJRange64i HJ_RANGE_ZERO = {0, 0};

typedef enum HJ_AMPEG_ID
{
    HJ_AMPEG_4 = 0,
    HJ_AMPEG_2 = 1,
} HJ_AMPEG_ID;

typedef enum HJAACProfileType
{
   HJ_AAPROFILE_MAIN = 0,
   HJ_AAPROFILE_LC = 1,
   HJ_AAPROFILE_SSR = 2,
   HJ_AAPROFILE_LTP = 3,
   HJ_AAPROFILE_HE = 4,
   HJ_AAPROFILE_LD = 22,
   HJ_AAPROFILE_HEV2 = 28,
   HJ_AAPROFILE_ELD = 38,
   HJ_AAPROFILE_M2_LOW = 128,    //mpeg2 low
   HJ_AAPROFILE_M2_HE = 131,
} HJAACProfileType;

typedef enum HJAACSampleRateType
{
    HJ_AACSR_NONE = -1,
    HJ_AACSR_96K = 0x00,      //96000
    HJ_AACSR_88K = 0x01,      //88200
    HJ_AACSR_64K = 0x02,      //64000
    HJ_AACSR_48K = 0x03,      //48000
    HJ_AACSR_44K = 0x04,      //44100
    HJ_AACSR_32K = 0x05,      //32000
    HJ_AACSR_24K = 0x06,      //24000
    HJ_AACSR_22K = 0x07,      //22050
    HJ_AACSR_16K = 0x08,      //16000
    HJ_AACSR_12K = 0x09,      //12000
    HJ_AACSR_11K = 0x0A,      //11025
    HJ_AACSR_8K = 0x0B,       //8000     
} HJAACSampleRateType;

typedef enum HJRunState
{
    HJRun_NONE = 0x00,
    HJRun_Init,
    HJRun_Start,
    HJRun_Ready,
    HJRun_Running,
    HJRun_Pause,
    HJRun_EOF,
    HJRun_Stop,
    HJRun_Done,
    HJRun_Error,
    HJRun_RESERVED = 0xFFFF,
    //
    //HJRun_FLUSH = 0x10000,
    //HJRun_IDLE  = 0x20000,
} HJRunState;
HJEnumToStringFuncDecl(HJRunState);

typedef enum HJEOFState
{
    HJEOF_NONE = 0x00,
    HJEOF_PREPARE,
    HJEOF_END,
} HJEOFState;

typedef enum HJRenderState
{
    HJRender_NONE = 0x00,
    HJRender_Init,
    HJRender_Prepare,
    HJRender_Ready,
    HJRender_Running,
    HJRender_Done,
} HJRenderState;

typedef enum HJDemuxerState
{
    HJDemuxer_NONE = 0x00,
    HJDemuxer_Init,
    HJDemuxer_Running,
    HJDemuxer_End,
    HJDemuxer_Done,
} HJDemuxerState;

typedef struct HJDeviceAttribute
{
    HJDeviceType   m_devType = HJDEVICE_TYPE_NONE;
    int             m_devID = 0;
} HJDeviceAttribute;
//***********************************************************************************//
class HJMediaUtils
{
public:
    static HJTimeBase getSuitableTimeBase(const HJRational rate);
    static HJTimeBase checkTimeBase(const HJTimeBase tbase);
    static int64_t calcFrameTimeWithIdx(int idx, const HJTimeBase tbase, const HJRational rate) {
        return idx * tbase.den * rate.den / (tbase.num * (int64_t)rate.num);
    }
    static int64_t calcFrameDuration(const HJTimeBase tbase, const HJRational rate) {
        int64_t mixNum = tbase.num * (int64_t)rate.num;
        return mixNum ? tbase.den * rate.den / (mixNum) : 33;
    }
    //
    static HJRectf calculateRenderRect(const HJRectf targetRect, const HJSizei inSize, const HJVCropMode mode = HJ_VCROP_FIT);
    static HJRectf calculateRenderRect(const HJSizei targetSize, const HJSizei inSize, const HJVCropMode mode = HJ_VCROP_FIT);

    /**
    * h2645ps2nalu -- add 0x03
    * aac extradata parse -- avpriv_mpeg4audio_get_config2(2 Byte)
    * adts header parse -- avpriv_adts_header_parse(7 Byte)
    */
    static HJBuffer::Ptr avcc2annexb(uint8_t* data, size_t size);
    static const HJAACSampleRateType HJAACSamleRate2Type(const int samplerate);
    static int parseAACExtradata(uint8_t* data, size_t size, int& objectType, int& samplerateIndex, int& channelConfig);
    /**
    * @brief 生成AAC AudioSpecificConfig extradata (通常为2字节)
    * @param samplerate 采样率 (e.g., 48000, 44100)
    * @param channels 声道数 (e.g., 1=mono, 2=stereo)
    * @param profile AAC profile类型，默认为AAC-LC
    * @return HJ_OK成功，其他值失败
    */
    static HJBuffer::Ptr makeAACExtraData(int samplerate, int channels, HJAACProfileType profile = HJ_AAPROFILE_LC);

    static std::string makeLocalUrl(const std::string& localDir, const std::string& url);
    static std::string getLocalUrl(const std::string& localDir, const std::string& remoteUrl, const std::string& rid = "");
    static std::string checkMediaSuffix(const std::string& suffix);
    static std::vector<std::string> enumMediaFiles(const std::string& dir);
private:
    static HJRectf alignToWidth(const HJRectf targetRect, float ratio);
    static HJRectf alignToHeight(const HJRectf targetRect, float ratio);
};

NS_HJ_END