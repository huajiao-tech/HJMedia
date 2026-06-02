#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#include "HJMediaUtils.h"
#include "HJCommonInterface.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include <memory>
#include <string>
#include <vector>

class HJFaceSRWrapper;

NS_HJ_BEGIN

class HJVisualDataUI;
class HJTransferMediaData;

class HJUIItemSR : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemSR);
    
	HJUIItemSR();
	virtual ~HJUIItemSR();

	virtual int run() override;
	//virtual bool updateTitle() override;
private:
	void priDone();
	int priResetSR();
	int priRun();
	void priReleasePausedFrame();
	int m_inferUseGPU = 1; // 0: CPU, 1: GPU
	int m_srWrapperType = 0; // 0: RealESRGAN, 1: FSR, 2: RealCUGAN, 3: PlainUSR
	int m_srRealEsr = 0; // 0: realesr-general-x4v3, 1: realesr-animevideov3-x2, 2: realesrgan-x2plus
	int m_srRealEsrDenoiseIdx = 3; // 0:0.0, 1:0.2, 2:0.8, 3:0.5, 4:1.0
	int m_srRealCU = 0; // 0: conservative, 1: no-denoise
	int m_srMode = 2; // 0: FullSR, 1: Face, 2: FaceScale, 3: FullScale
	bool m_srEnabled = true;
	bool m_featherAlphaEnable = true;
	bool m_enableNativePostSRDisplayResize = true;
	float m_srAlphaRatio = 0.8f;
	int m_faceScaleProcPolicy = 3; // 0: Mipmap, 1: Bilinear, 2: Bicubic, 3: Lanczos3, 4: Lanczos4
	int m_faceScalePresetIndex = 1;
	int m_fullScalePresetIndex = 1;
	int m_preScaleWidth = 90;
	int m_preScaleHeight = 114;
	int m_srRatio = 2;
	int m_srThreadNums = 8;
	float m_fsrDenoiseStrength = 1.0f;
	float m_fsrSharpness = 1.0f;
	bool m_fsrEnableExtraSharpen = true;
	float m_fsrExtraSharpenBoost = 0.55f;
	int64_t m_srElapseMs = 0;
	int m_actualInputWidth = 0;
	int m_actualInputHeight = 0;
	int m_lastFaceScaleW = 0;
	int m_lastFaceScaleH = 0;
	int m_lastSRFaceWidth = 0;
	int m_lastSRFaceHeight = 0;
	int m_lastFaceTargetDisplayW = 0;
	int m_lastFaceTargetDisplayH = 0;
	int m_lastPadLeft = 0;
	int m_lastPadRight = 0;
	int m_lastPadTop = 0;
	int m_lastPadBottom = 0;
	GLuint m_fsrInputTexture = 0;

	std::shared_ptr<HJVisualDataUI> m_visualDataUI = nullptr;
	std::shared_ptr<HJFaceSRWrapper> m_faceSR = nullptr;
	std::shared_ptr<HJTransferMediaData> m_cachedPauseFrame = nullptr;
	bool m_isPaused = false;
	std::vector<HJUnifyWrapperRect> m_faceRectsUI;
};

NS_HJ_END



