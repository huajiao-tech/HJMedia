#pragma once

#include "HJPrerequisites.h"


NS_HJ_BEGIN

class HJRawImageDataInfo;
class HJTransferMediaData;

typedef enum HJResourceImgSeqStyle
{
	HJResourceImgSeqStyle_LOW,
	HJResourceImgSeqStyle_HEIGHT,
	HJResourceImgSeqStyle_MIX,
	HJResourceImgSeqStyle_TWO,
} HJResourceImgSeqStyle;

class HJResourceImageInfo
{
public:
	HJ_DEFINE_CREATE(HJResourceImageInfo);
	HJResourceImageInfo() = default;
	virtual ~HJResourceImageInfo();
	HJResourceImageInfo(unsigned char* i_data, int i_width, int i_height, int i_components) :
		data(i_data)
		, width(i_width)
		, height(i_height)
		, components(i_components)

	{

	}
	int width = 0;
	int height = 0;
	int components = 0;
	unsigned char *data = nullptr;
};

class HJResourceImgSeq final
{
public:
	HJ_DEFINE_CREATE(HJResourceImgSeq);
	HJResourceImgSeq();
	~HJResourceImgSeq() = default;
	static HJResourceImgSeq& Instance();

	std::shared_ptr<HJRawImageDataInfo> getImgInfo(HJResourceImgSeqStyle i_type = HJResourceImgSeqStyle_LOW);

	std::shared_ptr<HJTransferMediaData> getImgDataRGBA(HJResourceImgSeqStyle i_type = HJResourceImgSeqStyle_LOW);
	std::shared_ptr<HJTransferMediaData> getImgDataRGB(HJResourceImgSeqStyle i_type = HJResourceImgSeqStyle_LOW);
	std::shared_ptr<HJTransferMediaData> getImgDataYUV420(HJResourceImgSeqStyle i_type = HJResourceImgSeqStyle_LOW);
	std::shared_ptr<HJTransferMediaData> getImgDataYUVNV12(HJResourceImgSeqStyle i_type = HJResourceImgSeqStyle_LOW);

private:
	HJResourceImageInfo::Ptr priGetDataInfo(HJResourceImgSeqStyle i_type = HJResourceImgSeqStyle_LOW);
	std::vector<std::string>& getImgSeqPaths(HJResourceImgSeqStyle i_type);

	static std::string m_imgSeqUrl;
	static std::string m_imgSeqUrlLow;
	static std::string m_imgSeqUrlHeigh;
	static std::string m_imgSeqUrlTwo;
	int m_imgIdx = 0;
	std::vector<std::string> m_imgSeqPathsHeigh;
	std::vector<std::string> m_imgSeqPathsLow;
	std::vector<std::string> m_imgSeqPathsMix;
	std::vector<std::string> m_imgSeqPathsTwo;
};

NS_HJ_END



