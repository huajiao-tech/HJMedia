#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include "HJAsyncCache.h"

NS_HJ_BEGIN

typedef enum HJPlayerVideoCodecType
{
	HJPlayerVideoCodecType_SoftDefault = 0,
	HJPlayerVideoCodecType_OHCODEC = 1,
	HJPlayerVideoCodecType_VIDEOTOOLBOX = 2,
	HJPlayerVideoCodecType_MEDIACODEC = 3,
} HJPlayerVideoCodecType;

class HJMediaFrame;
class HJYuvTexCvt;
class HJGraphComPlayer;
class HJUIItemPlayerCom : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemPlayerCom);
    
	HJUIItemPlayerCom();
	virtual ~HJUIItemPlayerCom();

	virtual int run() override;
    
private:
	void priDone();
	int priTryGetParam();

	std::string m_url = "E:/video/yuv/dance1_504_896.yuv";

	std::shared_ptr<HJYuvTexCvt> m_yuvTexCvt = nullptr;

	HJAsyncCache<std::shared_ptr<HJMediaFrame>> m_asyncCache;
	std::shared_ptr<HJGraphComPlayer> m_player = nullptr;
};

NS_HJ_END



