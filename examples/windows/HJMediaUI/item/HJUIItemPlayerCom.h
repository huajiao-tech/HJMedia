#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include "HJAsyncCache.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "HJRteUtils.h"

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
class HJRenderCvt;
class HJTransferMediaData;
class HJMediaDataDraw;

class HJUIItemPlayerCom : public HJUIBaseItem
{
public:

	HJ_DEFINE_CREATE(HJUIItemPlayerCom);

	HJUIItemPlayerCom();
	virtual ~HJUIItemPlayerCom();

	virtual int run() override;
	virtual int renderEveryStart() override;
	virtual int renderEveryEnd() override;
private:
	void priDone();
	int priTryGetParam();

	//std::string m_url = "E:/video/yuv/dance1_504_896.yuv";

	std::shared_ptr<HJYuvTexCvt> m_yuvTexCvt = nullptr;

	HJAsyncCache<std::shared_ptr<HJMediaFrame>> m_asyncCache;
	std::shared_ptr<HJGraphComPlayer> m_player = nullptr;

	std::shared_ptr<HJRenderCvt> m_renderCvt = nullptr;

	HJAsyncCache<std::shared_ptr<HJTransferMediaData>> m_asyncCacheRender;
	std::shared_ptr<HJMediaDataDraw> m_mediaDataDraw = nullptr;

	bool m_isBlure = false;
	bool m_isDenoise = false;
	bool m_isSR = false;
	float m_srMatch = 1.0f;
	bool m_isFaceu = false;
	static std::string s_imageUrl;
	static std::string s_pngSeqUrl;
	static std::string s_playerUrl;
	static std::string s_splitSreenGiftUrl;
	static std::string s_faceuUrl;
	static bool s_useSingleUI;
	static int s_singleUIWidth;
	static int s_singleUIHeight;
	static HJRteGraphConstructorType s_configGraphType;
	static bool s_xMirror;
	static bool s_customerFilter;
	static std::string s_imageSeq;
	// Cached PlayerParams window geometry (this frame)
	ImVec2 m_paramsPos = ImVec2(0.0f, 0.0f);
	ImVec2 m_paramsSize = ImVec2(0.0f, 0.0f);
	bool m_hasParamsInfo = false;
	std::string m_jsonConfig;
	int m_statIdx = 0;
	int m_randomVal = 10;
};

NS_HJ_END



