//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMediaInfo.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
const HJUtilTime HJUtilTime::HJTimeZero = { 0, HJ_TIMEBASE_MS };
const HJUtilTime HJUtilTime::HJTimeNOTS = { HJ_NOTS_VALUE, HJ_TIMEBASE_9W };
const HJUtilTime HJUtilTime::HJTimeInvalid = { 0, {0, 0} };

//
const HJMediaTime HJMediaTime::HJMediaTimeZero = {{0, HJ_TIMEBASE_MS}, {0, HJ_TIMEBASE_MS}};

//***********************************************************************************//
HJUtilTime HJUtilTime::unified(const HJTimeBase& tbase) {
    int64_t a = av_time_to_time(m_value, { m_timeBase.num, m_timeBase.den }, { tbase.num, tbase.den });
    return HJUtilTime(a, tbase);
}

const int64_t HJTimeStamp::getPTS() {
    return av_time_to_time(m_pts, { m_timeBase.num, m_timeBase.den }, { 1, 1000 });
}
const int64_t HJTimeStamp::getDTS() {
    return av_time_to_time(m_dts, { m_timeBase.num, m_timeBase.den }, { 1, 1000 });
}
const int64_t HJTimeStamp::getDuration() {
    return av_time_to_time(m_duration, { m_timeBase.num, m_timeBase.den }, { 1, 1000 });
}

const std::string HJMediaUrl::STORE_KEY_SUBURLS = "sub_urls";
//***********************************************************************************//
HJMediaUrl::HJMediaUrl(const HJMediaUrl::Ptr& other)
{
    clone(other);
}

HJMediaUrl::HJMediaUrl(const std::string& url/* = ""*/)
    : m_url(HJUtilitys::AnsiToUtf8(url))
{
    if (!m_url.empty()) {
        //m_fmtName = av_guess_format(NULL, m_url.c_str(), NULL);
        if (HJUtilitys::startWith(m_url, "rtmp")) {
            m_fmtName = "flv";
        }
        m_urlHash = HJUtilitys::hash(m_url);
    }
}

HJMediaUrl::HJMediaUrl(const std::string& url, int loopCnt, int64_t startTime/* = HJ_NOTS_VALUE*/, int64_t endTime/* = HJ_NOTS_VALUE*/, int disableMFlag/* = HJMEDIA_TYPE_NONE*/)
    : m_url(HJUtilitys::AnsiToUtf8(url))
    , m_loopCnt(loopCnt)
    , m_startTime(startTime)
    , m_endTime(endTime)
    , m_disableMFlag(disableMFlag)
{

}

HJMediaUrl::~HJMediaUrl()
{
    clear();
}

HJMediaUrl::Ptr HJMediaUrl::makeMediaUrl(const std::string& url, int flag) {
    HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(url);
    mediaUrl->setDisableMFlag(flag);
    return std::move(mediaUrl);
}

//***********************************************************************************//
HJHWDevice::HJHWDevice(const HJDeviceType devType, int devID/* = 0*/)
{
    m_deviceType = devType;
    m_deviceID = devID;
}

HJHWDevice::HJHWDevice(const HJHND hwCtx)
{
    if (hwCtx) {
        AVBufferRef* hw_device_ctx = av_buffer_ref((AVBufferRef*)hwCtx);
        m_hwCtx = hwCtx;
    }
}

HJHWDevice::~HJHWDevice()
{
    if (m_hwCtx) {
        AVBufferRef* hw_device_ctx = (AVBufferRef*)m_hwCtx;
        av_buffer_unref(&hw_device_ctx);
        m_hwCtx = NULL;
    }
}

int HJHWDevice::init()
{
    if (HJDEVICE_TYPE_NONE == m_deviceType) {
        return HJErrFatal;
    }
    if (HJDEVICE_TYPE_TSCSDK == m_deviceType) {
        return HJ_OK;
    }
    AVHWDeviceType hwType = hj_device_type_map(m_deviceType);
    std::string device_id = HJ2STR(m_deviceID);
    const char* devName = NULL;
    if(AV_HWDEVICE_TYPE_CUDA == hwType ||
       AV_HWDEVICE_TYPE_VDPAU == hwType) {
        devName = device_id.c_str();
    }
    AVBufferRef* hw_device_ctx = NULL;
    if (av_hwdevice_ctx_create(&hw_device_ctx, hwType, devName, NULL, 0) < HJ_OK) {
        HJLoge("Failed to create specified HW device.");
        return HJErrHWDevice;
    }
    m_hwCtx = hw_device_ctx;

    return HJ_OK;
}
//***********************************************************************************//
HJHWFrameCtx::HJHWFrameCtx(const HJHND frameRef)
{
    if (frameRef) {
        AVBufferRef* hw_frames_ref = av_buffer_ref((AVBufferRef *)frameRef);
        m_hwFrameRef = hw_frames_ref;
    }
}

HJHWFrameCtx::HJHWFrameCtx(const HJHWDevice::Ptr dev, int pixFmt, int swFmt, int width, int height, int poolSize/* = 0*/)
{
    AVBufferRef* hw_device_ctx = (AVBufferRef*)dev->getHWDeviceCtx();
    AVBufferRef* hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx);
	if (!hw_frames_ref) {
		HJLoge("Failed to create HW frame context. error");
		return;
	}
    AVHWFramesContext* frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
	frames_ctx->format = (enum AVPixelFormat)pixFmt;
    frames_ctx->sw_format = (enum AVPixelFormat)swFmt; //AV_PIX_FMT_NV12;
	frames_ctx->width = width;
	frames_ctx->height = height;
	frames_ctx->initial_pool_size = poolSize;

    int err = av_hwframe_ctx_init(hw_frames_ref);
	if (err < HJ_OK) {
		HJLoge("Failed to initialize hw frame context. error");
		av_buffer_unref(&hw_frames_ref);
	}
    m_hwFrameRef = hw_frames_ref;
}

HJHWFrameCtx::~HJHWFrameCtx()
{
	if (m_hwFrameRef) {
		AVBufferRef* hw_frames_ref = (AVBufferRef*)m_hwFrameRef;
		av_buffer_unref(&hw_frames_ref);
        m_hwFrameRef = NULL;
	}
}
//***********************************************************************************//
HJCodecParameters::HJCodecParameters()
{

}
HJCodecParameters::HJCodecParameters(const AVCodecParameters* codecParams, const bool isDup)
{
    m_codecParams = isDup ? av_dup_codec_params(codecParams) : (AVCodecParameters *)codecParams;
}

HJCodecParameters::HJCodecParameters(const HJCodecParameters::Ptr& other)
{
    clone(other);
}

HJCodecParameters::~HJCodecParameters()
{
    if (m_codecParams) {
        avcodec_parameters_free(&m_codecParams);
        m_codecParams = NULL;
    }
}

void HJCodecParameters::clone(const HJCodecParameters::Ptr& other)
{
    if (!other) {
        return;
    }
    m_codecParams = av_dup_codec_params(other->getAVCodecParameters());
}

bool HJCodecParameters::isEqual(const HJCodecParameters::Ptr& other)
{
    if (!m_codecParams || !other || !other->m_codecParams) {
        return false;
    }
    return av_codec_params_compare(m_codecParams, other->m_codecParams);
}

//***********************************************************************************//
const std::string HJStreamInfo::KEY_WORLDS = HJ_TYPE_NAME(HJStreamInfo);
const std::string HJStreamInfo::STORE_KEY_AVCODECPARMS = HJ_TYPE_NAME(AVCodecParameters);
const std::string HJStreamInfo::STORE_KEY_HWDEVICE = HJ_TYPE_NAME(HJHWDevice); //AVBufferRef
const std::string HJStreamInfo::type2String(int type)
{
    static const std::unordered_map<HJMediaType, std::string> g_mediaTypeTable = {
        {HJMEDIA_TYPE_NONE, "none"},
        {HJMEDIA_TYPE_VIDEO, "video"},
        {HJMEDIA_TYPE_AUDIO, "audio"},
        {HJMEDIA_TYPE_DATA, "data"},
        {HJMEDIA_TYPE_SUBTITLE, "subtitle"},
    };
    auto it = g_mediaTypeTable.find((HJMediaType)type);
    return (it != g_mediaTypeTable.end()) ? it->second : "";
}

AVCodecParameters* HJStreamInfo::makeCodecParameters(const HJStreamInfo::Ptr& info)
{
    if (!info) {
        return NULL;
    }
    AVCodecParameters* codecParams = NULL;
    if (HJ_ISTYPE(*info, HJAudioInfo)) 
    {
        HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(info);
    } else if (HJ_ISTYPE(*info, HJVideoInfo)) 
    {
        HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(info);
        codecParams = avcodec_parameters_alloc();
        codecParams->codec_type = AVMEDIA_TYPE_VIDEO;
        codecParams->codec_id = (AVCodecID)videoInfo->m_codecID;
        codecParams->codec_tag = 0;
        codecParams->format = videoInfo->m_pixFmt;
        codecParams->bit_rate = videoInfo->m_bitrate;
        codecParams->bits_per_coded_sample = 0;
        codecParams->bits_per_raw_sample = 0;
        codecParams->width = videoInfo->m_width;
        codecParams->height = videoInfo->m_height;
        codecParams->color_range = (enum AVColorRange)videoInfo->m_colorRange; //AVCOL_RANGE_UNSPECIFIED;
        codecParams->color_primaries = (enum AVColorPrimaries)videoInfo->m_colorPrimaries; // AVCOL_PRI_UNSPECIFIED;
        codecParams->color_trc = (enum AVColorTransferCharacteristic)videoInfo->m_colorTrc; // AVCOL_TRC_UNSPECIFIED;
        codecParams->color_space = (enum AVColorSpace)videoInfo->m_colorSpace; // AVCOL_SPC_UNSPECIFIED;
        codecParams->chroma_location = (enum AVChromaLocation)videoInfo->m_chromaLocation; // AVCHROMA_LOC_UNSPECIFIED;
    }
    return codecParams;
}

//***********************************************************************************//
HJStreamInfo::HJStreamInfo()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJStreamInfo)));
}

HJStreamInfo::~HJStreamInfo()
{
    clear();
}

void HJStreamInfo::clone(const HJStreamInfo::Ptr& other) {
    m_type = other->m_type;
    m_subType = other->m_subType;
    m_dataType = other->m_dataType;
    m_codecID = other->m_codecID;
    m_codecName = other->m_codecName;
    m_codecDevName = other->m_codecDevName;
    m_cert = other->m_cert;
    m_bitrate = other->m_bitrate;
    m_TimeBase = other->m_TimeBase;
    m_timeRange = other->m_timeRange;
    m_devType = other->m_devType;
    m_trackID = other->m_trackID;
    m_streamIndex = other->m_streamIndex;
    //
    HJKeyStorage::clone(other);
    //setCodecParams(other->getCodecParams());
}

HJCodecParameters::Ptr HJStreamInfo::getCodecParams()
{
    auto it = find(STORE_KEY_AVCODECPARMS);
    if (it != end()) {
        return std::any_cast<HJCodecParameters::Ptr>(it->second);
    }
    return nullptr;
}

void HJStreamInfo::setCodecParams(const HJCodecParameters::Ptr codecParams)
{
    if (codecParams) {
        (*this)[STORE_KEY_AVCODECPARMS] = codecParams;
    }
}

AVCodecParameters* HJStreamInfo::getAVCodecParams()
{
    const auto codecParam = getCodecParams();
    if (codecParam) {
        return codecParam->getAVCodecParameters();
    }
    return NULL;
}

void HJStreamInfo::setAVCodecParams(const AVCodecParameters* codecParams)
{
    if (!codecParams) {
        return;
    }
    const auto params = std::make_shared<HJCodecParameters>(codecParams);
    setCodecParams(params);
    return;
}

HJHWDevice::Ptr HJStreamInfo::getHWDevice()
{
    HJHWDevice::Ptr hwDev = nullptr;
    auto it = find(STORE_KEY_HWDEVICE);
    if (it != end()) {
        hwDev = std::any_cast<HJHWDevice::Ptr>(it->second);
    }
    return hwDev;
}

void HJStreamInfo::setHWDevice(const HJHWDevice::Ptr device)
{
    if (device) {
        (*this)[STORE_KEY_HWDEVICE] = device;
    }
}

const std::string HJStreamInfo::formatInfo()
{
    std::string info = "type:" + HJStreamInfo::type2String(m_type) + ", start time:" + HJ2STR(m_timeRange.getStart().getMS())
        + ", duration:" + HJ2STR(m_timeRange.getDuration().getMS());
    return info;
}

//***********************************************************************************//
HJChannelLayout::HJChannelLayout(const int channel)
{
    if (!channel) {
        return;
    }
    AVChannelLayout* chLayout = (AVChannelLayout*)av_mallocz(sizeof(AVChannelLayout));
    if (!chLayout) {
        return;
    }
    av_channel_layout_default(chLayout, channel);
    m_avchLayout = chLayout;
}

HJChannelLayout::HJChannelLayout(const AVChannelLayout* avchLayout)
{
    if (avchLayout) {
        m_avchLayout = av_dup_channel_layout((const AVChannelLayout *)avchLayout);
    }
}

HJChannelLayout::~HJChannelLayout()
{
    AVChannelLayout* chLayout = (AVChannelLayout*)m_avchLayout;
    if (chLayout) {
		av_channel_layout_uninit(chLayout);
		av_freep(&chLayout);
    }
}

//***********************************************************************************//
const std::string HJAudioInfo::STORE_KEY_HJChannelLayout = HJ_TYPE_NAME(HJChannelLayout);
HJAudioInfo::HJAudioInfo()
    : HJStreamInfo() 
{
	m_type = HJMEDIA_TYPE_AUDIO;
	m_bitrate = 96 * 1000;
	//
//	m_channelLayout = av_get_default_channel_layout(m_channels);
}

HJAudioInfo::~HJAudioInfo()
{
}

void HJAudioInfo::cloneAudioInfo(const HJAudioInfo::Ptr& other) {
    m_channels = other->m_channels;
    m_bytesPerSample = other->m_bytesPerSample;
    m_sampleFmt = other->m_sampleFmt;
    m_samplesRate = other->m_samplesRate;
    m_blockAlign = other->m_blockAlign;
    m_samplesPerFrame = other->m_samplesPerFrame;
    //        m_channelLayout = other->m_channelLayout;
    m_sampleLayout = other->m_sampleLayout;
    m_sampleCnt = other->m_sampleCnt;
    //
    setChannelLayout(other->getChannelLayout());
}

void HJAudioInfo::setChannels(int channels, const AVChannelLayout* avchLayout)
{
    m_channels = channels;
    if (avchLayout) { //AVChannelLayout
        HJChannelLayout::Ptr chLayout = std::make_shared<HJChannelLayout>(avchLayout);
        (*this)[STORE_KEY_HJChannelLayout] = chLayout;
    } else {
		HJChannelLayout::Ptr chLayout = std::make_shared<HJChannelLayout>(m_channels);
		(*this)[STORE_KEY_HJChannelLayout] = chLayout;
    }
}

void HJAudioInfo::setChannels(const AVChannelLayout* layout)
{
    if(!layout) {
        return;
    }
    m_channels = layout->nb_channels;
    HJChannelLayout::Ptr chLayout = std::make_shared<HJChannelLayout>(layout);
    (*this)[STORE_KEY_HJChannelLayout] = chLayout;
    return;
}

AVChannelLayout* HJAudioInfo::getAVChannelLayout()
{
    AVChannelLayout* avchLayout = NULL;
    HJChannelLayout::Ptr chLayout = getChannelLayout();
    if (chLayout){
        avchLayout = (AVChannelLayout *)chLayout->getAVCHLayout();
    }
    return avchLayout;
}

bool HJAudioInfo::operator!=(const HJAudioInfo& other) const
{
    if (this == &other) {
        return false;
    }
    if (other.m_channels != m_channels ||
        other.m_samplesRate != m_samplesRate ||
        other.m_sampleFmt != m_sampleFmt) {
        return true;
    }
    return false;
}

const std::string HJAudioInfo::formatInfo()
{
    std::string info = HJStreamInfo::formatInfo();
    info += ", channels:" + HJ2STR(m_channels) + ", sample rate:" + HJ2STR(m_samplesRate) + ", sample bits:" + HJ2STR(m_bytesPerSample * 16/m_channels);
    return info;
}

//***********************************************************************************//
const std::string HJVideoInfo::STORE_KEY_HWFramesCtx = "HWFramesCtx";
const std::string HJVideoInfo::STORE_KEY_MediaCodecSurface = "mediacodec_surface";
//
HJVideoInfo::HJVideoInfo() 
    : HJStreamInfo() 
{
    m_type = HJMEDIA_TYPE_VIDEO;
    m_bitrate = 1000 * 1000;
    //
    m_pixFmt = AV_PIX_FMT_YUV420P;
    m_colorRange = AVCOL_RANGE_UNSPECIFIED;
    m_colorPrimaries = AVCOL_PRI_UNSPECIFIED;
    m_colorTrc = AVCOL_TRC_UNSPECIFIED;
    m_colorSpace = AVCOL_SPC_UNSPECIFIED;
    m_chromaLocation = AVCHROMA_LOC_UNSPECIFIED;
}
HJVideoInfo::HJVideoInfo(int width, int height) {
    m_type = HJMEDIA_TYPE_VIDEO;
    m_width = width;
    m_height = height;
    m_bitrate = 1000 * 1000;
    //
    m_pixFmt = AV_PIX_FMT_YUV420P;
}
HJVideoInfo::~HJVideoInfo()
{
    auto it = find(STORE_KEY_HWFramesCtx);
    if (it != end()) {
        //HJHWFrameCtx::Ptr hwFrameCtx = std::any_cast<HJHWFrameCtx::Ptr>(it->second);
        erase(it);
    }
}

void HJVideoInfo::cloneVideoInfo(const HJVideoInfo::Ptr& other) {
    m_width = other->m_width;
    m_height = other->m_height;
    for (int i = 0; i < 4; i++) {
        m_stride[i] = other->m_stride[i];
    }
    m_pixFmt = other->m_pixFmt;
    m_aspectRatio = other->m_aspectRatio;
    m_scaleMode = other->m_scaleMode;
    m_frameRate = other->m_frameRate;
    m_gopSize = other->m_gopSize;
    m_maxBFrames = other->m_maxBFrames;
    m_lookahead = other->m_lookahead;
    m_quality = other->m_quality;
    m_rotate = other->m_rotate;
    //
    m_colorRange = other->m_colorRange;
    m_colorPrimaries = other->m_colorPrimaries;
    m_colorTrc = other->m_colorTrc;
    m_colorSpace = other->m_colorSpace;
    m_chromaLocation = other->m_chromaLocation;
}

HJHWFrameCtx::Ptr HJVideoInfo::getHWFramesCtx()
{
    HJHWFrameCtx::Ptr hwFrameCtx = nullptr;
    auto it = find(STORE_KEY_HWFramesCtx);
    if (it != end()) {
        hwFrameCtx = std::any_cast<HJHWFrameCtx::Ptr>(it->second);
    }
    return hwFrameCtx;
}

HJHND HJVideoInfo::getMediaCodecSurface()
{
    HJHND surface = NULL;
    auto it = find(STORE_KEY_MediaCodecSurface);
    if (it != end()) {
        surface = std::any_cast<HJHND>(it->second);
    }
    return surface;
}

const std::string HJVideoInfo::formatInfo()
{
    std::string info = HJStreamInfo::formatInfo();
    info += ", width:" + HJ2STR(m_width) + ", height:" + HJ2STR(m_height) + ", framerate:" + HJ2STR(m_frameRate);
    return info;
}

//***********************************************************************************//
const std::string HJMediaInfo::KEY_WORLDS = typeid(HJMediaInfo).name();
HJMediaInfo::HJMediaInfo(const int mediaType/* = HJMEDIA_TYPE_NONE*/, const std::string& url/* = ""*/)
    : m_mediaTypes(mediaType)
{
    if(HJMEDIA_TYPE_VIDEO & mediaType) {
        HJVideoInfo::Ptr videoInfo = std::make_shared<HJVideoInfo>();
        videoInfo->setID(m_streamInfos.size());
        m_streamInfos.push_back(videoInfo);
    }
    if (HJMEDIA_TYPE_AUDIO & mediaType) {
        HJAudioInfo::Ptr audioInfo = std::make_shared<HJAudioInfo>();
        audioInfo->setID(m_streamInfos.size());
        m_streamInfos.push_back(audioInfo);
    }
    if(HJMEDIA_TYPE_SUBTITLE & mediaType) {
        HJSubtitleInfo::Ptr subInfo = std::make_shared<HJSubtitleInfo>();
        subInfo->setID(m_streamInfos.size());
        m_streamInfos.push_back(subInfo);
    }
    if (!url.empty()) {
        m_url = std::make_shared<HJMediaUrl>(url);
    }
}

HJMediaInfo::~HJMediaInfo()
{

}

int HJMediaInfo::forEachInfo(const std::function<int(const HJStreamInfo::Ptr &)>& cb)
{
    int res = HJ_OK;
    for (auto &info : m_streamInfos) {
        res = cb(info);
        if (HJ_OK != res) {
            break;
        }
    }
    return res;
}

const std::string HJMediaInfo::formatInfo()
{
    std::string info = "{stream cnt:" + HJ2STR(m_streamInfos.size()) + ", start time:" + HJ2STR(m_timeRange.getStart().getMS())
    + ", duration:" + HJ2STR(m_timeRange.getDuration().getMS()) + " [";
    for (auto &stream : m_streamInfos) {
        info += " {id:" + HJ2STR(stream->getID()) + ", " + stream->formatInfo() + "},";
    }
    info += " ]}";
    return info;
}

//***********************************************************************************//
const std::string HJSeekInfo::KEY_WORLDS = typeid(HJSeekInfo).name();
HJSeekInfo::HJSeekInfo(const int64_t pos/* = 0*/)
    : m_pos(pos)
{
    m_id = HJMakeGlobalID();
    m_time = HJCurrentSteadyMS();
}

//***********************************************************************************//
const std::string HJProgressInfo::KEY_WORLDS = typeid(HJProgressInfo).name();


NS_HJ_END
