#include "HJFLog.h"
#include "HJRenderCvt.h"
#include "HJRteGraphProc.h"
#include "HJTransferMediaData.h"
#include "HJRteComDraw.h"
#include "HJOGFBOCtrl.h"
#include "HJOGBaseShader.h"
#include "HJRteGraphSetupInfo.h"
#include "HJGUICommon.h"
#include "HJRteGraphNodeInfo.h"
#include "HJComEvent.h"

NS_HJ_BEGIN

HJRenderCvt::HJRenderCvt()
{
}
HJRenderCvt::~HJRenderCvt()
{
	if (m_rteGraphProc)
	{
		m_rteGraphProc->done();
		m_rteGraphProc = nullptr;
	}
	if (m_uiWindow)
	{
		glfwDestroyWindow(m_uiWindow);
		m_uiWindow = nullptr;
	}
}
void HJRenderCvt::openPngseq(const std::string& i_url)
{
	if (m_rteGraphProc)
	{
		HJBaseParam::Ptr param = HJBaseParam::Create();
		HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlImgSeq, i_url);
		m_rteGraphProc->openPNGSeq(param);
	}
}
void HJRenderCvt::setFaceInfo(const std::string& i_sourceInsName, const std::string& i_faceInfo)
{
	if (m_rteGraphProc)
	{
		m_rteGraphProc->setFaceInfo(i_sourceInsName, i_faceInfo);
	}
}
void HJRenderCvt::setLinkRenderCallback(std::function<void(const std::string&, const std::string&, const std::string&, bool i_bOnlyCopy)> cb)
{
	m_linkRenderCallback = std::move(cb);
}
void HJRenderCvt::setFrameStatCallback(std::function<void(int64_t timeMs, double fps, double renderAvgMs)> cb)
{
	m_frameStatCallback = std::move(cb);
}
int HJRenderCvt::nodeCreate(const std::string& classStyle, const std::string& insName, const std::string& role, bool enable, bool isMainSource, const std::string& i_resourceUrl, const std::string& i_dependsOn)
{
    if (!m_rteGraphProc)
    {
        return HJErrInvalid;
    }
    HJRteJsonNode node(classStyle, insName, role, enable, isMainSource, i_resourceUrl, i_dependsOn);
    return m_rteGraphProc->nodeCreate(node);
}
int HJRenderCvt::nodeDelete(const std::string& classStyle, const std::string& insName, bool relink)
{
    if (!m_rteGraphProc)
    {
        return HJErrInvalid;
    }
    return m_rteGraphProc->nodeDelete(classStyle, insName, relink);
}
int HJRenderCvt::nodeConnect(const std::string& srcClass, const std::string& srcIns,
                             const std::string& dstClass, const std::string& dstIns,
                             const std::string& shaderType, HJRteJsonLinkInfo& linkInfo)
{
    if (!m_rteGraphProc)
    {
        return HJErrInvalid;
    }
    return m_rteGraphProc->nodeConnect(srcClass, srcIns, dstClass, dstIns, shaderType, linkInfo);
}
int HJRenderCvt::nodeDisconnect(const std::string& srcClass, const std::string& srcIns,
                                const std::string& dstClass, const std::string& dstIns,
                                const std::string& linkId)
{
    if (!m_rteGraphProc)
    {
        return HJErrInvalid;
    }
    return m_rteGraphProc->nodeDisconnect(srcClass, srcIns, dstClass, dstIns, linkId);
}
int HJRenderCvt::nodeEnable(const std::string& classStyle, const std::string& insName, bool enable, const std::string& info)
{
    if (!m_rteGraphProc)
    {
        return HJErrInvalid;
    }
    return m_rteGraphProc->nodeEnable(classStyle, insName, enable, info);
}
int HJRenderCvt::nodeSetParam(const std::string& classStyle, const std::string& insName, HJBaseParam::Ptr i_param)
{
    if (!m_rteGraphProc)
    {
        return HJErrInvalid;
    }
    return m_rteGraphProc->nodeSetParam(classStyle, insName, i_param);
}
int HJRenderCvt::nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
	const std::string& i_dstClassStyle, const std::string& i_dstInsName,
	HJRteJsonLinkInfo& i_linkInfo)
{
	if (!m_rteGraphProc)
	{
		return HJErrInvalid;
	}
	return m_rteGraphProc->nodeLinkInfoChange(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkInfo);
}
void HJRenderCvt::setFaceu(bool i_bFaceu, const std::string& i_url, bool i_bDebugPoint, bool i_bUseFakePoint)
{
    if (m_rteGraphProc)
    { 
#if 0
		if (i_bFaceu)
        { 
			HJRteJsonNode::Ptr faceNode = HJRteJsonNode::Create<HJRteJsonNode>(HJRteGraphConfig::HJNodeClass_SourceFaceu,
				HJRteGraphConfig::HJNodeClass_SourceFaceu,
				HJRteGraphConfig::HJNodeRole_Source, false);
			m_rteGraphProc->nodeCreate(faceNode->initSerial());

			HJRteJsonLinkInfo::Ptr link = HJRteJsonLinkInfo::Create();

			m_rteGraphProc->nodeConnect(HJRteGraphConfig::HJNodeClass_SourceFaceu,
				HJRteGraphConfig::HJNodeClass_SourceFaceu,
				HJRteGraphConfig::HJNodeClass_TargetUI_0,
				HJRteGraphConfig::HJNodeClass_TargetUI_0,
				HJRteGraphConfig::HJNodeShaderType_Copy2D,
				link->initSerial());

			m_rteGraphProc->nodeConnect(HJRteGraphConfig::HJNodeClass_SourceFaceu,
				HJRteGraphConfig::HJNodeClass_SourceFaceu,
				HJRteGraphConfig::HJNodeClass_TargetPBO_0,
				HJRteGraphConfig::HJNodeClass_TargetPBO_0,
				HJRteGraphConfig::HJNodeShaderType_Copy2D,
				link->initSerial());

			HJRteGraphConfigFaceuInfo info;
			info.bDebugPoints = i_bDebugPoint;
			info.bUseFakePoints = i_bUseFakePoint;
			info.faceuUrl = i_url;
			m_rteGraphProc->setEnable(HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu, i_bFaceu, info.initSerial());
        }
		else
		{
			m_rteGraphProc->nodeDelete(HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu);
		}

#endif

		//if faceu use placeholder
		m_rteGraphProc->nodeEnable(HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu, i_bFaceu, i_url);

#if 0
		if (i_bFaceu)
		{
			HJBaseParam::Ptr param = HJBaseParam::Create();
			HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlFaceu, i_url);
			HJ_CatchMapPlainSetVal(param, bool, "bUseFakePoints", i_bUseFakePoint);
			HJ_CatchMapPlainSetVal(param, bool, "bDebugPoint", i_bDebugPoint);

			
			bool bUseSharedCtxTexture = true;
			HJ_CatchMapPlainSetVal(param, bool, "bUseSharedCtxTexture", bUseSharedCtxTexture);
			if (bUseSharedCtxTexture)
			{
				//not correct, may be thread, m_uiWindow is use render thread, is not makecurrent; only if use lock m_uiWindow context;
				glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
				glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API); //windows use WGL , other use EGL
				GLFWwindow* offscreenWindow = glfwCreateWindow(1, 1, "faceuOffscreenWindow", NULL, m_parentWindow);// (GLFWwindow*)m_uiWindow);

				HJThreadTaskFunc funcInit = [offscreenWindow]()
					{
						glfwMakeContextCurrent(offscreenWindow);
						return 0;
					};
				HJThreadTaskFunc funcEnd = [offscreenWindow]()
					{
						glfwDestroyWindow(offscreenWindow);
						//glfwMakeContextCurrent(nullptr);
						return 0;
					};
				HJ_CatchMapPlainSetVal(param, HJThreadTaskFunc, "faceuInitFunc", funcInit);
				HJ_CatchMapPlainSetVal(param, HJThreadTaskFunc, "faceuEndFunc", funcEnd);
			}
			

			m_rteGraphProc->openFaceu(param);
		}
		else
		{
			m_rteGraphProc->closeFaceu();
		}
#endif
	}
}
void HJRenderCvt::setBlur(bool i_blur)
{
	if (m_rteGraphProc)
	{
		HJBaseParam::Ptr param = HJBaseParam::Create();
		HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJRteEffectType), HJRteEffect_Blur);
		if (i_blur)
		{
			m_rteGraphProc->openEffect(param);
		}
		else
		{
			m_rteGraphProc->closeEffect(param);
		} 
	}
}
int HJRenderCvt::setDenoise(bool i_enable, float i_strength)
{
	if (!m_rteGraphProc)
	{
		return HJErrInvalid;
	}

	HJBaseParam::Ptr param = HJBaseParam::Create();
	HJ_CatchMapPlainSetVal(param, float, "denoiseStrength", i_strength);

	int i_err = m_rteGraphProc->nodeSetParam(HJRteGraphConfig::HJNodeClass_FilterDenoise, HJRteGraphConfig::HJNodeClass_FilterDenoise, param);
	if (i_err < 0)
	{
		HJFLoge("HJRenderCvt::setDenoise setParam error:{}", i_err);
		return i_err;
	}

	i_err = m_rteGraphProc->nodeEnable(HJRteGraphConfig::HJNodeClass_FilterDenoise, HJRteGraphConfig::HJNodeClass_FilterDenoise, i_enable, "");
	if (i_err < 0)
	{
		HJFLoge("HJRenderCvt::setDenoise nodeEnable error:{}", i_err);
	}
	return i_err;
}
int HJRenderCvt::setSR(bool i_enable, float i_sharpness, float i_match)
{
	if (!m_rteGraphProc)
	{
		return HJErrInvalid;
	}

	HJBaseParam::Ptr param = HJBaseParam::Create();
	HJ_CatchMapPlainSetVal(param, float, "sharpness", i_sharpness);
	HJ_CatchMapPlainSetVal(param, float, "match", i_match);

	int i_err = m_rteGraphProc->nodeSetParam(HJRteGraphConfig::HJNodeClass_FilterSR, HJRteGraphConfig::HJNodeClass_FilterSR, param);
	if (i_err < 0)
	{
		HJFLoge("HJRenderCvt::setSR setParam error:{}", i_err);
		return i_err;
	}

	i_err = m_rteGraphProc->nodeEnable(HJRteGraphConfig::HJNodeClass_FilterSR, HJRteGraphConfig::HJNodeClass_FilterSR, i_enable, "");
	if (i_err < 0)
	{
		HJFLoge("HJRenderCvt::setSR nodeEnable error:{}", i_err);
	}
	return i_err;
}
void HJRenderCvt::priAddCustomFilter(HJBaseParam::Ptr io_param, bool i_bcustomerFilter)
{
	HJ_CatchMapPlainSetVal(io_param, bool, "useCustomFilter", i_bcustomerFilter);
	if (i_bcustomerFilter)
	{
		HJRteComDrawGrayFBO::Ptr grayFbo = HJRteComDrawGrayFBO::Create();
		HJRteComDrawXMirrorFBO::Ptr xMirrorFbo = HJRteComDrawXMirrorFBO::Create();
		
		HJCustomSourceFilterInit initFunc = [grayFbo, xMirrorFbo]()
			{
				int i_err = HJ_OK;
				do 
				{
					i_err = grayFbo->init(nullptr);
					if (i_err < 0)
					{
						break;
					}
					i_err = xMirrorFbo->init(nullptr);
					if (i_err < 0)
					{
						break;
					}
				} while (false);
				return i_err;
			};
		HJCustomSourceFilterDraw drawFunc = [this, grayFbo, xMirrorFbo](GLuint i_inTextureId, GLuint& outTextureId, int i_srcWidth, int i_srcHeight)
			{
				outTextureId = i_inTextureId;
				if (!grayFbo || !xMirrorFbo)
				{
					return -1;
				}
				grayFbo->reset();
                xMirrorFbo->reset();

				grayFbo->refCntSet(1);
                xMirrorFbo->refCntSet(1);

				grayFbo->adjustResolution(i_srcWidth, i_srcHeight);
				xMirrorFbo->adjustResolution(i_srcWidth, i_srcHeight);
				HJRteDriftInfo::Ptr outDriftInfo1 = nullptr;
				{
					HJRteComLink::Ptr link = HJRteComLink::Create();
					HJRteDriftInfo::Ptr inDriftInfo = HJRteDriftInfo::Create<HJRteDriftInfo>("customInput", i_inTextureId, HJRteTextureType_2D, i_srcWidth, i_srcHeight);
					grayFbo->bind();
					grayFbo->render(link, inDriftInfo);
					grayFbo->unbind(true);

					outDriftInfo1 = HJRteDriftInfo::Create<HJRteDriftInfo>(grayFbo->getInsName(), grayFbo->getTextureId(), HJRteTextureType_2D, i_srcWidth, i_srcHeight);
					
					{
						auto fbo = grayFbo->takeFbo();
						if (fbo)
						{
							auto lease = HJFboLease::Create<HJFboLease>(grayFbo->getInsName(), HJRteGraphBaseEGL::getFBOCtrlPool(), std::move(fbo));
							outDriftInfo1->attachFboLease(lease);
						}
					}
					grayFbo->setDriftInfo(outDriftInfo1);
				}

				outDriftInfo1 = grayFbo->getDriftInfo();
				if (!outDriftInfo1)
				{
					return -1;
				}
				HJRteDriftInfo::Ptr outDriftInfo2 = nullptr;
				{
					HJRteComLink::Ptr link = HJRteComLink::Create();
					HJRteDriftInfo::Ptr inDriftInfo = HJRteDriftInfo::Create<HJRteDriftInfo>("grayInput", outDriftInfo1->m_textureId, outDriftInfo1->m_textureType, outDriftInfo1->m_srcWidth, outDriftInfo1->m_srcHeight);
					xMirrorFbo->bind();
					xMirrorFbo->render(link, inDriftInfo);
					xMirrorFbo->unbind(true);

					outDriftInfo2 = HJRteDriftInfo::Create<HJRteDriftInfo>(xMirrorFbo->getInsName(), xMirrorFbo->getTextureId(), HJRteTextureType_2D, i_srcWidth, i_srcHeight);
					xMirrorFbo->setDriftInfo(outDriftInfo2);

					{
						auto fbo = xMirrorFbo->takeFbo();
						if (fbo)
						{
							auto lease = HJFboLease::Create<HJFboLease>(xMirrorFbo->getInsName(), HJRteGraphBaseEGL::getFBOCtrlPool(), std::move(fbo));
							outDriftInfo2->attachFboLease(lease);
						}
					}
				}

				bool bCreateFbo = false;
				if (!m_customFBOCtrl)
				{
					bCreateFbo = true;
				}
				else if (m_customFBOCtrl->getWidth() != i_srcWidth || m_customFBOCtrl->getHeight() != i_srcHeight)
				{
					bCreateFbo = true;
				}
				if (bCreateFbo)
				{
					m_customFBOCtrl = HJRteGraphBaseEGL::getFBOCtrlPool()->acquire("customFBOCtrl", i_srcWidth, i_srcHeight);// HJOGFBOCtrl::Create();
					//m_customFBOCtrl->init(i_srcWidth, i_srcHeight);
				}
				if (!m_customShader)
				{
					m_customShader = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
				}
				outDriftInfo2 = xMirrorFbo->getDriftInfo();
				if (outDriftInfo2)
				{
					m_customFBOCtrl->attach();
					m_customShader->draw(outDriftInfo2->m_textureId, HJWindowRenderMode_CLIP, i_srcWidth, i_srcHeight, i_srcWidth, i_srcHeight);
					m_customFBOCtrl->detach();
					outTextureId = m_customFBOCtrl->getTextureId();
				}
				
				return 0;
			};	
		HJCustomSourceFilterRelease releaseFunc = [this, grayFbo, xMirrorFbo]()
			{	
				if (grayFbo)
				{
					grayFbo->done();
				}
                if (xMirrorFbo)
				{
					xMirrorFbo->done();
				}	
				m_customFBOCtrl = nullptr;
				m_customShader = nullptr;
				return 0;
			};
		HJ_CatchMapSetVal(io_param, HJCustomSourceFilterInit, initFunc);
		HJ_CatchMapSetVal(io_param, HJCustomSourceFilterDraw, drawFunc);
		HJ_CatchMapSetVal(io_param, HJCustomSourceFilterRelease, releaseFunc);
	}
	
}

int HJRenderCvt::init(HJBaseParam::Ptr io_param)
{
	int i_err = 0;
	do
	{
		void * pWindow = nullptr;
		HJ_CatchMapPlainGetVal(io_param, void*, "ParentWindow", pWindow);
		m_parentWindow = (GLFWwindow *)pWindow;
		//if (!m_offscreenWindow)
		//{
		//	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		//	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API); //windows use WGL , other use EGL
		//	m_offscreenWindow = glfwCreateWindow(1, 1, "offscreenWindow", NULL, (GLFWwindow *)pWindow);
		//	//glfwMakeContextCurrent(m_offscreenWindow);
		//	//glfwMakeContextCurrent(nullptr);
		//}
		int width = 1;
		int height = 1;
		bool bSingleUI = false;
		if (!m_uiWindow)
		{
			HJ_CatchMapPlainGetVal(io_param, bool, "IsUseSingleUI", bSingleUI);
			HJ_CatchMapPlainGetVal(io_param, int, "SingleUIWidth", width);
			HJ_CatchMapPlainGetVal(io_param, int, "SingleUIHeight", height);

			if (bSingleUI)
			{
				glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
			}
			else
			{
				glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

			}
			
			glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API); //windows use WGL , other use EGL
			m_uiWindow = glfwCreateWindow(width, height, "ui", NULL, m_parentWindow);// also can use shared context with parent window NULL=>(GLFWwindow*)pWindow;

			if (!m_uiWindow)
			{
				glfwTerminate();
				i_err = -1;
				break;
			}


			HJGUICommon::allignMonitorTopRight(m_uiWindow, width, height);

			//main thread, must glfwMakeContextCurrent(nullptr), and after render thread can use makecurrent
			//glfwMakeContextCurrent(m_uiWindow);  //if makecurrent(window), must makecurrent(nullptr), else other thread can't use makecurrent
			//glfwMakeContextCurrent(nullptr);
		}
		if (!m_rteGraphProc)
		{
			bool bcustomerFilter = false;
			std::string imageUrl = "";
			HJTransferMediaDataGetCb getMDataCb = nullptr;
			HJ_CatchMapGetVal(io_param, HJTransferMediaDataGetCb, getMDataCb);
			HJ_CatchMapGetVal(io_param, HJTransferMediaDataSetCb, m_renderMediaDataSetCb);
			HJ_CatchMapPlainGetVal(io_param, std::string, "ImageUrl", imageUrl);
			HJRteGraphConstructorType configGraphType = HJRteGraphConstructorType_None;
			HJ_CatchMapGetVal(io_param, HJRteGraphConstructorType, configGraphType);
			HJ_CatchMapPlainGetVal(io_param, bool, "UseCustomerFilter", bcustomerFilter);
			bool bSplitScreenXMirror = false;
			HJ_CatchMapPlainGetVal(io_param, bool, "IsMirror", bSplitScreenXMirror);

			m_rteGraphProc = HJRteGraphProc::createRteGraphProc((HJRteGraphProcType)HJRteGraphProcType_CONFIG_SETUP);

			HJBaseParam::Ptr param = HJBaseParam::Create();
			HJ_CatchMapPlainSetVal(param, bool, "IsMirror", bSplitScreenXMirror);
			HJ_CatchMapSetVal(param, HJTransferMediaDataGetCb, getMDataCb);

			std::string imageSeq = "";
			HJ_CatchMapPlainGetVal(io_param, std::string, HJRteUtils::ParamUrlImgSeq, imageSeq);
			if (!imageSeq.empty())
			{
				HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlImgSeq, imageSeq);
			}

			bool bMainSourceRenderForce = true;
			HJ_CatchMapPlainGetVal(io_param, bool, "bMainSourceRenderForce", bMainSourceRenderForce);
			HJ_CatchMapPlainSetVal(param, bool, "bMainSourceRenderForce", bMainSourceRenderForce);

			HJMediaDataReaderCb pboReadCb = [this](HJSPBuffer::Ptr i_buffer, int width, int height)
				{
					HJTransferMediaDataRGBA::Ptr mData = HJTransferMediaDataRGBA::Create();
					mData->setBuffer(i_buffer);
					mData->setWidth(width);
					mData->setHeight(height);
					if (m_renderMediaDataSetCb)
					{
						m_renderMediaDataSetCb(mData);
					}
					HJFLogd("HJRenderCvt::pboReadCb width:{} height{}", width, height);
					return 0;
				};
			HJ_CatchMapSetVal(param, HJMediaDataReaderCb, pboReadCb);

			//HJRenderInitCb renderInit = [this]()
			//{
			//	glfwMakeContextCurrent(m_offscreenWindow);
			//	return 0;
			//};
			//HJRenderEveryStartCb renderEveryStart = [this]()
			//{
			//	GLenum err = glGetError();
			//	if (err != GL_NO_ERROR)
			//	{
			//		HJFLoge("HJRenderCvt::renderEveryStart glGetError:{}", err);
			//	}
			//	glfwMakeContextCurrent(m_offscreenWindow);
			//	//glClearColor(0, 0, 0, 0);
			//	//glClear(GL_COLOR_BUFFER_BIT);
			//	return 0;
			//};
   //         HJRenderEveryEndCb renderEveryEnd = [this]()
			//{
			//	HJFLogi("HJRenderCvt::renderEveryEnd");
			//	//glfwSwapBuffers(m_offscreenWindow);
			//	return 0;
			//};
			//HJ_CatchMapSetVal(param, HJRenderInitCb, renderInit);
			//HJ_CatchMapSetVal(param, HJRenderEveryStartCb, renderEveryStart);
			//HJ_CatchMapSetVal(param, HJRenderEveryEndCb, renderEveryEnd);

			HJRenderMakeCurrent makeCurrentCb = [this](bool i_bMake)
				{
					if (i_bMake)
					{
						glfwMakeContextCurrent(m_uiWindow);
					}
					else
					{
						glfwMakeContextCurrent(nullptr);
					}
					return HJ_OK;
				};
			HJ_CatchMapSetVal(param, HJRenderMakeCurrent, makeCurrentCb);

			priAddCustomFilter(param, bcustomerFilter);

			std::string configInfo = "";
			HJ_CatchMapPlainGetVal(io_param, std::string, "UserConfig", configInfo);

			if (configInfo.empty())
			{
				HJ_CatchMapSetVal(param, HJRteGraphConstructorType, configGraphType);
				configInfo = HJRteGraphConfigConstructor::constructGraph(param);
			}
			if (!imageUrl.empty())
			{
				HJ_CatchMapPlainSetVal(param, std::string, "ImageUrl", imageUrl);
			}
			HJ_CatchMapPlainSetVal(param, std::string, "graphConfigInfo", configInfo);

			HJRteGraphLinkRenderCb linkRenderCb = [this](const std::string& linkId, const std::string& from, const std::string& to, bool i_bOnlyCopy)
				{
					if (m_linkRenderCallback)
					{
						m_linkRenderCallback(linkId, from, to, i_bOnlyCopy);
					}
				};
			HJ_CatchMapSetVal(param, HJRteGraphLinkRenderCb, linkRenderCb);
			HJRteGraphFrameStatCb frameStatCb = [this](int64_t timeMs, double fps, double renderAvgMs)
				{
					if (m_frameStatCallback)
					{
						m_frameStatCallback(timeMs, fps, renderAvgMs);
					}
				};
			HJ_CatchMapSetVal(param, HJRteGraphFrameStatCb, frameStatCb);

			HJListener renderListener = [&](const HJNotification::Ptr ntf) -> int
				{
					if (!ntf)
					{
						return HJ_OK;
					}
					HJFLogi("graph video capture notify id:{}, msg:{}", HJVideoRenderGraphEventToString((HJVideoRenderGraphEvent)ntf->getID()), ntf->getMsg());
					//int type = HJVIDEORENDERGRAPH_EVENT_NONE;
					//std::string msg = ntf->getMsg();
					//switch (ntf->getID())
					//{
				
					//default:
					//{
					//	break;
					//}
					//}
					//if (m_notify && (HJVIDEORENDERGRAPH_EVENT_NONE != type))
					//{
					//	m_notify(type, msg);
					//}
					return HJ_OK;
				};
			(*param)["renderListener"] = (HJListener)renderListener;

			i_err = m_rteGraphProc->init(param);
			if (i_err < 0)
			{
				break;
			}

			if (bSingleUI)
			{
				m_surfacePtr = HJOGEGLSurface::Create();
				m_surfacePtr->setInsName("windows_eglsurface");
				m_surfacePtr->setWindow(m_uiWindow);
				m_surfacePtr->setEGLSurface(m_uiWindow);
				m_surfacePtr->setTargetWidth(width);
				m_surfacePtr->setTargetHeight(height);
				m_surfacePtr->setSurfaceType(HJOGEGLSurfaceType_UI);
				m_surfacePtr->setFps(30);
				m_surfacePtr->setMakeCurrentCb([this]()
					{
						glfwMakeContextCurrent(m_uiWindow);
						return HJ_OK;
					});
				m_surfacePtr->setSwapCb([this]()
					{
						glfwSwapBuffers(m_uiWindow);
						return HJ_OK;
					});

				m_rteGraphProc->procEGLSurface(m_surfacePtr, HJOGEGLSurfaceType_UI, HJTargetState_Create);
			}

			HJ_CatchMapPlainSetVal(io_param, std::string, "graphConfigInfo", configInfo);
		}
	} while (false);
	return i_err;
	return 0;
}

NS_HJ_END



