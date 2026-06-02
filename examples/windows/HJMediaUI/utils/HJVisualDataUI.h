#pragma once

#include "HJPrerequisites.h"
#include <memory>
#include "HJComUtils.h"
#include "HJConvertUtils.h"

NS_HJ_BEGIN

class VisualDataBase;
class HJTransferMediaData;

class HJVisualDataUI : public HJBaseSharedObject
{
public:  
	HJ_DEFINE_CREATE(HJVisualDataUI);

	enum SourceType
	{
		SourceType_Video = 0,
		SourceType_Camera = 1,
		SourceType_ImgSeq = 2,
		SourceType_Image = 3
	};

	HJVisualDataUI();
	virtual ~HJVisualDataUI();

	int applySource(HJConvertDataType i_dataType);
	int drawUI(const char* i_windowName = "VisualData");
	std::shared_ptr<HJTransferMediaData> acquire();
	void recovery(std::shared_ptr<HJTransferMediaData> i_data);

	std::shared_ptr<VisualDataBase> getVisualData() const;
	void stop();

private:
	int m_sourceType = SourceType_Camera;
	int m_fps = 30;
	int m_cameraWidth = 360;// 480;// 720;
	int m_cameraHeight = 640;// 848;// 1280;

	char m_videoUrl[512] = {};
	char m_cameraUrl[512] = {};
	char m_imgSeqUrl[512] = {};
	char m_imageUrl[512] = {};

	std::shared_ptr<VisualDataBase> m_visualData = nullptr;
};

NS_HJ_END



