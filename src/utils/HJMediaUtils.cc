//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMediaUtils.h"
#include "HJUtilitys.h"

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
	objectType = (data[0] >> 3) & 0x1F;
	samplerateIndex = ((data[0] & 0x01) << 1) | ((data[1] >> 7) & 0x01);
	channelConfig = (data[1] >> 3) & 0x0F;
	return HJ_OK;
}

NS_HJ_END

