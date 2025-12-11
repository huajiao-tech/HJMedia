//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMediaUtils.h"
#include "HJUtilitys.h"
#include "HJFileUtil.h"
#include "HJFLog.h"
#include <unordered_set>

NS_HJ_BEGIN
//***********************************************************************************//
HJEnumToStringFuncImplBegin(HJRunState)
	HJEnumToStringItem(HJRun_NONE),
	HJEnumToStringItem(HJRun_Init),
	HJEnumToStringItem(HJRun_Start),
	HJEnumToStringItem(HJRun_Ready),
	HJEnumToStringItem(HJRun_Running),
	HJEnumToStringItem(HJRun_Pause),
	HJEnumToStringItem(HJRun_Stop),
	HJEnumToStringItem(HJRun_Done),
	HJEnumToStringItem(HJRun_Error),
	HJEnumToStringItem(HJRun_RESERVED),
HJEnumToStringFuncImplEnd(HJRunState);

//***********************************************************************************//
HJTimeBase HJMediaUtils::getSuitableTimeBase(const HJRational rate)
{
	HJTimeBase tbase = HJ_TIMEBASE_9W;
	double step = tbase.den* rate.num / (tbase.num * tbase.den);
	bool isInteger = HJUtilitys::isInteger(step);
	if (!isInteger) {
		int64_t den = rate.num * rate.den;
		while (den < HJ_TIMEBASE_9W.den) {
			den *= 10;
		}
		tbase.num = 1;
		tbase.den = den;
	}
	return tbase;
}

// normal HJ_TIMEBASE_9W
HJTimeBase HJMediaUtils::checkTimeBase(const HJTimeBase inTBase)
{
	HJTimeBase outTBase = inTBase;
	double fraction = outTBase.den * (double)HJ_TIMEBASE_9W.num / (outTBase.num * (double)HJ_TIMEBASE_9W.den);
	while (fraction < 1.0) {
		outTBase.den *= 10.0;
		fraction = outTBase.den * (double)HJ_TIMEBASE_9W.num / (outTBase.num * (double)HJ_TIMEBASE_9W.den);
	}
	return outTBase;
}

HJRectf HJMediaUtils::calculateRenderRect(const HJRectf targetRect, const HJSizei inSize, const HJVCropMode mode)
{
	HJRectf rect = HJ_RECT_DEFAULT;
	float rs = inSize.w / (float)inSize.h;
	float rw = targetRect.w / (float)targetRect.h;
	switch (mode)
	{
	case HJ_VCROP_FIT: {
		rect = (rs > rw) ? alignToWidth(targetRect, rs) : alignToHeight(targetRect, rs);
		break;
	}
	case HJ_VCROP_CLIP: {
		rect = (rs > rw) ? alignToHeight(targetRect, rs) : alignToWidth(targetRect, rs);
		break;
	}		 
	case HJ_VCROP_ORIGINAL: {
		rect = {0.0f, 0.0f, (float)inSize.w, (float)inSize.h};
		break;
	}
	case HJ_VCROP_FILL:
	default:
		rect = targetRect;
		break;
	}
	return rect;
}

HJRectf HJMediaUtils::calculateRenderRect(const HJSizei targetSize, const HJSizei inSize, const HJVCropMode mode)
{
	HJRectf targetRect = { 0.0f, 0.0f, (float)targetSize.w, (float)targetSize.h };
	return calculateRenderRect(targetRect, inSize, mode);
}

HJRectf HJMediaUtils::alignToWidth(const HJRectf targetRect, float ratio)
{
	HJRectf rect;
	rect.w = targetRect.w;
	rect.h = targetRect.w / ratio;
	rect.x = targetRect.x;
	rect.y = targetRect.y + (targetRect.h - rect.h) * 0.5f;
	return rect;
}

HJRectf HJMediaUtils::alignToHeight(const HJRectf targetRect, float ratio)
{
	HJRectf rect;
	rect.w = targetRect.h * ratio;
	rect.h = targetRect.h;
	rect.x = targetRect.x + (targetRect.w - rect.w) * 0.5f;
	rect.y = targetRect.y;
	return rect;
}

HJBuffer::Ptr HJMediaUtils::avcc2annexb(uint8_t* data, size_t size)
{
	if (!data || size <= 0) {
		return nullptr;
	}
	HJBuffer::Ptr annexb = std::make_shared<HJBuffer>(size);
	uint8_t* pdata = (uint8_t*)data;
	while (annexb->size() < size)
	{
		uint32_t nalLen = HJ_BE_32(pdata);
		annexb->wb32(0x00000001);
		annexb->write((const uint8_t*)(pdata + 4), nalLen);
		pdata += nalLen + 4;
	}
	return annexb;
}

static const std::unordered_map<int, HJAACSampleRateType> g_AACSamleRate2Type =
{
	{96000, HJ_AACSR_96K},
	{88200, HJ_AACSR_88K},
	{64000, HJ_AACSR_64K},
	{48000, HJ_AACSR_48K},
	{44100, HJ_AACSR_44K},
	{32000, HJ_AACSR_32K},
	{24000, HJ_AACSR_24K},
	{22050, HJ_AACSR_22K},
	{16000, HJ_AACSR_16K},
	{12000, HJ_AACSR_12K},
	{11025, HJ_AACSR_11K},
	{8000, HJ_AACSR_8K},
};

const HJAACSampleRateType HJMediaUtils::HJAACSamleRate2Type(const int samplerate)
{
	auto it = g_AACSamleRate2Type.find(samplerate);
	if (it != g_AACSamleRate2Type.end()) {
		return it->second;
	}
	return HJ_AACSR_NONE;
}

int HJMediaUtils::parseAACExtradata(uint8_t* data, size_t size, int& objectType, int& samplerateIndex, int& channelConfig)
{
	// AudioSpecificConfig 结构:
	// byte 0: [audioObjectType(5)] [samplingFrequencyIndex高3位(3)]
	// byte 1: [samplingFrequencyIndex低1位(1)] [channelConfiguration(4)] [reserved(3)]
	objectType = (data[0] >> 3) & 0x1F;
	samplerateIndex = ((data[0] & 0x07) << 1) | ((data[1] >> 7) & 0x01);  // 4位: 高3位来自byte0低3位，低1位来自byte1最高位
	channelConfig = (data[1] >> 3) & 0x0F;
	return HJ_OK;
}
/**
 * @brief 生成AAC AudioSpecificConfig extradata
 * 
 * AudioSpecificConfig 结构 (ISO 14496-3):
 * - 5 bits: audioObjectType (AAC-LC = 2)
 * - 4 bits: samplingFrequencyIndex
 * - 4 bits: channelConfiguration  
 * - 3 bits: 保留位 (设置为0)
 * 
 * 例如 48kHz stereo AAC-LC: 0x11 0x90
 *   audioObjectType = 2 (0b00010)
 *   samplingFrequencyIndex = 3 (0b0011) for 48000Hz
 *   channelConfiguration = 2 (0b0010) for stereo
 *   结果: 0b00010_001 0b1_0010_000 = 0x11 0x90
 */
HJBuffer::Ptr HJMediaUtils::makeAACExtraData(int samplerate, int channels, HJAACProfileType profile)
{
    // 获取采样率索引
    auto srType = HJMediaUtils::HJAACSamleRate2Type(samplerate);
    if (HJ_AACSR_NONE == srType) {
        HJFLoge("Unsupported AAC sample rate: {}", samplerate);
        return nullptr;
    }
    int samplingFrequencyIndex = (int)srType;

    // audioObjectType: AAC-LC=2, AAC-Main=1, AAC-SSR=3, AAC-LTP=4
    // profile枚举值与audioObjectType关系: profile + 1 = audioObjectType
    int audioObjectType = (int)profile + 1;
    int channelConfiguration = channels;

    // 分配 extradata 内存 (2字节 + padding)
    const int AAC_EXTRADATA_SIZE = 2;
	HJBuffer::Ptr extraBuffer = std::make_shared<HJBuffer>(AAC_EXTRADATA_SIZE);
	uint8_t* extradata = extraBuffer->data();
    if (!extradata) {
        HJLoge("Failed to allocate AAC extradata");
        return nullptr;
    }

    // 构建 AudioSpecificConfig
    // byte 0: [audioObjectType(5)] [samplingFrequencyIndex高3位(3)]
    // byte 1: [samplingFrequencyIndex低1位(1)] [channelConfiguration(4)] [reserved(3)]
    extradata[0] = ((audioObjectType & 0x1F) << 3) | ((samplingFrequencyIndex >> 1) & 0x07);
    extradata[1] = ((samplingFrequencyIndex & 0x01) << 7) | ((channelConfiguration & 0x0F) << 3);

	extraBuffer->setSize(AAC_EXTRADATA_SIZE);

    HJFLogi("AAC extradata generated: 0x{:02x} 0x{:02x} (sr:{}, ch:{}, profile:{})", 
		extradata[0], extradata[1], samplerate, channels, audioObjectType);

    return extraBuffer;
}

std::string HJMediaUtils::makeLocalUrl(const std::string& localDir, const std::string& url)
{
	auto coreUrl = HJUtilitys::getCoreUrl(url);
	auto suffix = checkMediaSuffix(HJUtilitys::getSuffix(coreUrl));
	auto localUrl = HJUtilitys::concatenatePath(localDir, HJFMT("{}{}", HJUtilitys::hash(url), suffix));
	return localUrl;
}

std::string HJMediaUtils::getLocalUrl(const std::string& localDir, const std::string& remoteUrl, const std::string& rid)
{
    auto suffix = HJMediaUtils::checkMediaSuffix(HJUtilitys::getSuffix(remoteUrl));
    std::string name = rid.empty() ? HJ2STR(HJUtilitys::hash(remoteUrl)) : rid;
	auto localUrl = HJUtilitys::concatenatePath(localDir, HJFMT("{}{}", name, suffix));
    return localUrl;
}

std::string HJMediaUtils::checkMediaSuffix(const std::string& suffix)
{
	static std::unordered_set<std::string> suffixes = {
		"mp4", "m4a", "m4v", "3gp", "mov", "mj2", "mkv", "webm", "ts", "mts", "m2ts", "vob", "ogv", "ogg", "wmv", "avi", "flv", "m4s", "m3u8", "mp3", "aac", "wav", "wma", "m4b", "m4p", "m4r", "m4v", "m4a", "m4e", "m4x", "m4y", "m4z", "m4w", "m4h", "m4i", "m4j", "m4k", "m4l", "m4m", "m4"
	};
	std::string cleanSuffix = suffix;
	if (!suffix.empty() && suffix[0] == '.') {
		cleanSuffix = suffix.substr(1);
	}

	if (suffixes.find(cleanSuffix) != suffixes.end()) {
		return suffix;
	}
	return ".mp4";  //default
}

std::vector<std::string> HJMediaUtils::enumMediaFiles(const std::string& dir)
{
	std::vector<std::string> mediaFiles;
	if (!HJFileUtil::isDirExist(dir)) {
		return mediaFiles;
	}
	for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
		if (entry.is_regular_file()) {
			std::string filename = entry.path().filename().string();
			mediaFiles.push_back(filename);
		}
	}
	return mediaFiles;
}

NS_HJ_END

