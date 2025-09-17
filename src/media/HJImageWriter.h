//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseCodec.h"
#include "HJVideoConverter.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJImageWriter : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJImageWriter>;
	HJImageWriter();
	virtual ~HJImageWriter();

	int init(const HJVideoInfo::Ptr& videoInfo);
	int initWithJPG(const int width, const int height, const float quality = 0.8f);
	int initWithPNG(const int width, const int height);
	int writeFrame(const HJMediaFrame::Ptr frame, const std::string& url);
private:
	std::string				m_url = "";
	HJVideoInfo::Ptr		m_info = nullptr;
	HJBaseCodec::Ptr		m_encoder = nullptr;
	HJVSwsConverter::Ptr	m_converter = nullptr;
    size_t                  m_frameIdx = 0;
};

NS_HJ_END
