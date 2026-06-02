#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include "HJAsyncCache.h"
#include <GLFW/glfw3.h>
#include "HJTransferMediaData.h"
#include "HJRteGraphSetupInfo.h"
#include <functional>

NS_HJ_BEGIN

class HJRteGraphProc;
class HJOGEGLSurface;
class HJOGFBOCtrl;
class HJOGBaseShader;

class HJRenderCvt
{
public:  
	HJ_DEFINE_CREATE(HJRenderCvt);
	HJRenderCvt();
	virtual ~HJRenderCvt();
	int init(HJBaseParam::Ptr io_param);

	void setBlur(bool i_blur);
	int setDenoise(bool i_enable, float i_strength = 1.0f);
	int setSR(bool i_enable, float i_sharpness = 1.0f, float i_match = 1.0f);
	void setFaceu(bool i_bFaceu, const std::string& i_url, bool i_bDebugPoint, bool i_bUseFakePoint);
	void openPngseq(const std::string& i_url);
    // Dynamic graph operations
    int nodeCreate(const std::string& classStyle, const std::string& insName, const std::string& role, bool enable, bool isMainSource,
		const std::string &i_resourceUrl = "", const std::string &i_dependsOn = "");
    int nodeDelete(const std::string& classStyle, const std::string& insName, bool relink);
    int nodeConnect(const std::string& srcClass, const std::string& srcIns,
                    const std::string& dstClass, const std::string& dstIns,
                    const std::string& shaderType, HJRteJsonLinkInfo& linkInfo);
    int nodeDisconnect(const std::string& srcClass, const std::string& srcIns,
                       const std::string& dstClass, const std::string& dstIns,
                       const std::string& linkId = "");
    int nodeEnable(const std::string& classStyle, const std::string& insName, bool enable, const std::string& info = "");
    int nodeSetParam(const std::string& classStyle, const std::string& insName, HJBaseParam::Ptr i_param);
    int nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
		const std::string& i_dstClassStyle, const std::string& i_dstInsName,
		HJRteJsonLinkInfo& linkInfo);
    void setFaceInfo(const std::string& i_sourceInsName, const std::string& i_faceInfo);
    void setLinkRenderCallback(std::function<void(const std::string&, const std::string&, const std::string&, bool i_bOnlyCopy)> cb);
    void setFrameStatCallback(std::function<void(int64_t timeMs, double fps, double renderAvgMs)> cb);
private:
	void priAddCustomFilter(HJBaseParam::Ptr io_param, bool i_bCustomFilter);
	//GLFWwindow* m_offscreenWindow= nullptr;
	GLFWwindow* m_uiWindow = nullptr;
	GLFWwindow* m_parentWindow = nullptr;
	std::shared_ptr<HJRteGraphProc> m_rteGraphProc = nullptr;
	HJTransferMediaDataSetCb m_renderMediaDataSetCb = nullptr; 
	std::shared_ptr<HJOGEGLSurface> m_surfacePtr = nullptr;

	std::shared_ptr<HJOGFBOCtrl> m_customFBOCtrl = nullptr; 
	std::shared_ptr<HJOGBaseShader> m_customShader = nullptr;
    std::function<void(const std::string&, const std::string&, const std::string&, bool i_bOnlyCopy)> m_linkRenderCallback = nullptr;
    std::function<void(int64_t timeMs, double fps, double renderAvgMs)> m_frameStatCallback = nullptr;
};

NS_HJ_END



