#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include "HJAsyncCache.h"

class HJRenderGraphWrapper;
class HJFaceDetectWrapper;

NS_HJ_BEGIN

class HJMediaDataDraw;
class HJTransferMediaData;

class HJUIItemRendGraphWrapper : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemRendGraphWrapper);
    
	HJUIItemRendGraphWrapper();
	virtual ~HJUIItemRendGraphWrapper();

	virtual int run() override;
	virtual bool updateTitle() override;
private:
	void priDone();
	int priResetRender();
	int priResetFaceDetect();
	int priCloseRender();
	int priCloseDetect();
	void priRenderImg();
	std::shared_ptr<HJRenderGraphWrapper> m_renderGraph = nullptr; 
	std::shared_ptr<HJFaceDetectWrapper> m_faceDetect = nullptr;
	HJAsyncCache<std::shared_ptr<HJTransferMediaData>> m_asyncCacheRender;
	std::shared_ptr<HJMediaDataDraw> m_mediaDataDraw = nullptr;
	int m_faceDetectSyncMode = 0;
	int m_faceDetectSmooth = 0;
	int m_faceDetectType = 3;
	int m_faceDetectDebugImage = 0;
	int m_faceDetectDebugPoints = 0;
	int m_ncnnRetinaFaceThread = 1;
	int m_ncnnRetinaFaceUseGPU = 0;

	static std::string m_faceuUrl0;
	static std::string m_faceuUrl1;
	static std::string m_faceuUrl2;

	std::mutex m_faceMutex;
	std::string m_cacheFaceInfo = "";

	int64_t m_elapseImgGetTime = 0;
	int64_t m_elapseFaceDetectTime = 0;
};

NS_HJ_END



