#include "HJFLog.h"
#include "HJResourceImgSeq.h"
#include "HJImgSeqInfo.h"
#include "stb_image.h"
#include "HJTransferMediaData.h"
#include "HJTime.h"

NS_HJ_BEGIN

std::string HJResourceImgSeq::m_imgSeqUrlLow = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/imgseq/sing_low");
std::string HJResourceImgSeq::m_imgSeqUrlHeigh = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/imgseq/sing");
std::string HJResourceImgSeq::m_imgSeqUrlTwo = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/imgseq/two");

HJResourceImageInfo::~HJResourceImageInfo()
{
	if (data)
	{
		stbi_image_free(data);
	}
}

HJResourceImgSeq::HJResourceImgSeq()
{
	HJImgSeqConfigInfo infoLow;
	m_imgSeqPathsLow = HJImgSeqParse::parseConfig(m_imgSeqUrlLow, infoLow);
	HJFLogi("m_imgSeqPaths size: {}", m_imgSeqPathsLow.size());

	HJImgSeqConfigInfo infoHeigh;
	m_imgSeqPathsHeigh = HJImgSeqParse::parseConfig(m_imgSeqUrlHeigh, infoHeigh);
	HJFLogi("m_imgSeqPaths size: {}", m_imgSeqPathsHeigh.size());

	m_imgSeqPathsMix = m_imgSeqPathsLow;
	m_imgSeqPathsMix.insert(m_imgSeqPathsMix.end(), m_imgSeqPathsHeigh.begin(), m_imgSeqPathsHeigh.end());
	HJFLogi("m_imgSeqPaths size: {}", m_imgSeqPathsMix.size());

	HJImgSeqConfigInfo infoTwo;
	m_imgSeqPathsTwo = HJImgSeqParse::parseConfig(m_imgSeqUrlTwo, infoTwo);
	HJFLogi("m_imgSeqPathsTwo size: {}", m_imgSeqPathsTwo.size());
}
HJResourceImgSeq& HJResourceImgSeq::Instance()
{
	static HJResourceImgSeq instance;
	return instance;
}

std::vector<std::string>& HJResourceImgSeq::getImgSeqPaths(HJResourceImgSeqStyle i_type)
{
	switch (i_type)
	{
	case HJResourceImgSeqStyle_HEIGHT:
		return m_imgSeqPathsHeigh;
	case HJResourceImgSeqStyle_MIX:
		return m_imgSeqPathsMix;
	case HJResourceImgSeqStyle_LOW:
		return m_imgSeqPathsLow;
	case HJResourceImgSeqStyle_TWO:
		return m_imgSeqPathsTwo;
	default:
		return m_imgSeqPathsLow;
	}
}

HJResourceImageInfo::Ptr HJResourceImgSeq::priGetDataInfo(HJResourceImgSeqStyle i_type)
{
	int width = 0;
	int height = 0;
	int components = 0;
	int64_t t0 = HJCurrentSteadyMS();
	std::vector<std::string>& vec = getImgSeqPaths(i_type);
	std::string url = vec[m_imgIdx];
	unsigned char* data = stbi_load(url.c_str(), &width, &height, &components, 0);
	if (!data)
	{
		return nullptr;
	}
	//HJFLogi("image decode time:{}", (HJCurrentSteadyMS() - t0));
	m_imgIdx++;
	if (m_imgIdx >= vec.size())
	{
		m_imgIdx = 0;
	}
	return HJResourceImageInfo::Create<HJResourceImageInfo>(data, width, height, components);
}

std::shared_ptr<HJRawImageDataInfo> HJResourceImgSeq::getImgInfo(HJResourceImgSeqStyle i_type)
{
	HJRawImageDataInfo::Ptr rawInfo = nullptr;
	int width = 0;
	int height = 0;
	int nrComponents = 0;

	std::vector<std::string>& vec = getImgSeqPaths(i_type);

	unsigned char* data = stbi_load(vec[m_imgIdx].c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{

		rawInfo = HJRawImageDataInfo::Create();
		rawInfo->m_imgIdx = m_imgIdx;
		rawInfo->m_width = width;
		rawInfo->m_height = height;
		rawInfo->m_components = nrComponents;
		rawInfo->m_buffer = HJSPBuffer::create(width * height * nrComponents, data);
		//HJFLogi("{} decode png end idx:{} loop:{} fps:{} prefix:{}", m_insName, m_pngIdx, m_configInfo.loops, m_configInfo.fps, m_configInfo.prefix);
		stbi_image_free(data);
	}
	m_imgIdx++;
	if (m_imgIdx >= vec.size())
	{
		m_imgIdx = 0;
	}

	return rawInfo;
}
std::shared_ptr<HJTransferMediaData> HJResourceImgSeq::getImgDataRGBA(HJResourceImgSeqStyle i_type)
{
	HJResourceImageInfo::Ptr imgPtr = HJResourceImgSeq::priGetDataInfo(i_type);
	if (!imgPtr)
	{
		return nullptr;
	}
	return nullptr;

}
std::shared_ptr<HJTransferMediaData> HJResourceImgSeq::getImgDataRGB(HJResourceImgSeqStyle i_type)
{
	HJResourceImageInfo::Ptr imgPtr = HJResourceImgSeq::priGetDataInfo(i_type);
	if (!imgPtr)
	{
		return nullptr;
	}
	return nullptr;
}
std::shared_ptr<HJTransferMediaData> HJResourceImgSeq::getImgDataYUV420(HJResourceImgSeqStyle i_type)
{
	HJResourceImageInfo::Ptr imgPtr = HJResourceImgSeq::priGetDataInfo(i_type);
	if (!imgPtr)
	{
		return nullptr;
	}
	return nullptr;
}
std::shared_ptr<HJTransferMediaData> HJResourceImgSeq::getImgDataYUVNV12(HJResourceImgSeqStyle i_type)
{
	int64_t t0 = HJCurrentSteadyMS();
	HJResourceImageInfo::Ptr imgPtr = HJResourceImgSeq::priGetDataInfo(i_type);
	if (!imgPtr)
	{
		return nullptr;
	}

	HJConvertDataType inType = imgPtr->components == 3 ? HJConvertDataType_RGB : HJConvertDataType_RGBA;
	HJConvertDataType outType = HJConvertDataType_YUVNV12;
	unsigned char* inData[4] = {imgPtr->data, nullptr, nullptr, nullptr};
	int inPitch[4] = { imgPtr->width * imgPtr->components, 0, 0, 0 };
	HJTransferMediaData::Ptr data = HJTransferMediaData::create(inType, inData, inPitch, imgPtr->width, imgPtr->height, HJCurrentSteadyMS(), outType);
	int64_t t1 = HJCurrentSteadyMS();
	//data->setElapseMs(t1 - t0);
	HJ_CatchMapPlainSetVal(data, int64_t, "ImgDecodeElapse", (t1 - t0));

	return data;
}

NS_HJ_END



