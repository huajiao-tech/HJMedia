#include "HJRenderFaceuMgr.h"
#include "HJError.h"
#include "HJFLog.h"

#include "HJOGFBOCtrl.h"
#include "HJRenderCoreSingleton.h"
#include "HJRteComSource.h"
#include "HJOGBaseShader.h"
#include "HJPBOReadWrapper.h"
#include "HJRenderFaceuObject.h"

#define SAVE_YUV 0

NS_HJ_BEGIN

HJRenderFaceuEnvMgr::HJRenderFaceuEnvMgr()
{
}
HJRenderFaceuEnvMgr::~HJRenderFaceuEnvMgr()
{
}

int HJRenderFaceuEnvMgr::init(HJFaceuListener i_listener)
{
	int i_err = HJ_OK;
	do
	{
		i_err = HJRenderFaceuBaseMgr::init(i_listener);
		if (i_err < 0)
		{
			break;
		}

		
	} while (false);
    return i_err;
}

bool HJRenderFaceuEnvMgr::priIsAdjustResolution(int i_width, int i_height)
{
	bool bAdjust = false;
	do
	{
		if ((m_width != i_width) || (m_height != i_height))
		{
			HJFLogd("update oldWidth:{} oldHeight:{} newWidth:{} newHeight:{}", m_width, m_height, i_width, i_height);
			m_width = i_width;
			m_height = i_height;
			bAdjust = true;
		}
	} while (false);
	return bAdjust;
}
void HJRenderFaceuEnvMgr::priTryCreatePBO()
{
#if SAVE_YUV
	m_pbReadWrapper = HJPBOReadWrapper::Create<HJPBOReadWrapper>(HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "tst.yuv"));
#else
	m_pbReadWrapper = HJPBOReadWrapper::Create();
#endif
	m_pbReadWrapper->setReadCb([this](HJSPBuffer::Ptr i_buffer, int width, int height)
		{
			m_outBuffer = std::move(i_buffer);
			return HJ_OK;
		});
}
int HJRenderFaceuEnvMgr::render(int i_width, int i_height, std::shared_ptr<HJMoreFacePointsReal> i_points, unsigned char *&o_pRGBA)
{
	int i_err = HJ_OK;

	o_pRGBA = nullptr;
	m_outBuffer = nullptr;

	i_err = HJRenderCoreSingleton::Instance().makeContext();
    if (i_err != HJ_OK)
    {
        HJFLoge("HJRenderFaceuBaseMgr::draw() HJRenderCoreSingleton::Instance().makeContext() failed");
        return i_err;
    }
	do
	{
		if (priIsAdjustResolution(i_width, i_height))
		{
			m_fboCtrl = HJOGFBOCtrl::Create();
			i_err = m_fboCtrl->init(i_width, i_height);
			if (i_err != HJ_OK)
            {
				HJFLoge("HJRenderFaceuBaseMgr::draw() m_fboCtrl->init() failed");
                break;
            }	
		}

		m_fboCtrl->attach();
			
		i_err = HJRenderFaceuBaseMgr::render(i_width, i_height, i_points, o_pRGBA);
		if (i_err < 0)
        {
            HJFLoge("HJRenderFaceuBaseMgr::draw() HJRenderFaceuBaseMgr::render() failed");
			m_fboCtrl->detach();
            break;
        }
		if (i_err == HJ_WOULD_BLOCK)
		{
			m_pbReadWrapper = nullptr;
			//HJFLogi("would block not use pbo");
		}
		else
		{
			if (!m_pbReadWrapper)
			{
				priTryCreatePBO();
			}
			m_pbReadWrapper->process(i_width, i_height);
			if (m_outBuffer)
			{
				o_pRGBA = m_outBuffer->getBuf();
			}
		}
		m_fboCtrl->detach();
	} while (false);

	HJRenderCoreSingleton::Instance().makeCurrentNULL();

    return i_err;
}
int HJRenderFaceuEnvMgr::add(const std::string& i_uniqueKey, const std::string& i_url, bool i_bDebugPoints)
{
    int i_err = HJ_OK;
	i_err = HJRenderCoreSingleton::Instance().makeContext();
	if (i_err != HJ_OK)
	{
		HJFLoge("HJRenderFaceuBaseMgr::draw() HJRenderCoreSingleton::Instance().makeContext() failed");
		return i_err;
	}
	do
	{		
		i_err = HJRenderFaceuBaseMgr::add(i_uniqueKey, i_url, i_bDebugPoints);
        if (i_err != HJ_OK)
        {
            HJFLoge("HJRenderFaceuBaseMgr::add() HJRenderFaceuObject::init() failed");
            break;
        }
	} while (false);
	HJRenderCoreSingleton::Instance().makeCurrentNULL();
    return i_err;
}
int HJRenderFaceuEnvMgr::remove(const std::string& i_uniqueKey)
{
	int i_err = HJ_OK;
	i_err = HJRenderCoreSingleton::Instance().makeContext();
	if (i_err != HJ_OK)
	{
		HJFLoge("HJRenderFaceuBaseMgr::draw() HJRenderCoreSingleton::Instance().makeContext() failed");
		return i_err;
	}
	do
	{	
		i_err = HJRenderFaceuBaseMgr::remove(i_uniqueKey);
	} while (false);

	HJRenderCoreSingleton::Instance().makeCurrentNULL();
    return i_err;
}

int HJRenderFaceuEnvMgr::removeAll()
{
	int i_err = HJ_OK;
	i_err = HJRenderCoreSingleton::Instance().makeContext();
	if (i_err != HJ_OK)
	{
		HJFLoge("HJRenderFaceuBaseMgr::draw() HJRenderCoreSingleton::Instance().makeContext() failed");
		return i_err;
	}
	do
	{
		i_err = HJRenderFaceuBaseMgr::removeAll();
	} while (false);
	HJRenderCoreSingleton::Instance().makeCurrentNULL();
    return i_err;
}
void HJRenderFaceuEnvMgr::done()
{
	int i_err = HJ_OK;
	i_err = HJRenderCoreSingleton::Instance().makeContext();
	if (i_err != HJ_OK)
	{
		HJFLoge("HJRenderFaceuBaseMgr::draw() HJRenderCoreSingleton::Instance().makeContext() failed");
		return;
	}
	do
	{
		HJRenderFaceuBaseMgr::done();
		m_fboCtrl = nullptr;
        m_pbReadWrapper = nullptr;
        m_outBuffer = nullptr;
	} while (false);
	HJRenderCoreSingleton::Instance().makeCurrentNULL();
}
NS_HJ_END


//static HJRteComSourceImgSeq::Ptr pngSeq = nullptr;
		//if (!pngSeq)
		//{
		//	pngSeq = HJRteComSourceImgSeq::Create();
		//	HJBaseParam::Ptr param = HJBaseParam::Create();
		//	std::string imageSeq = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/imgseq/sing"); //"E:/code/git/hjmedia/examples/resource/pngseg/sing";
		//	HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlImgSeq , imageSeq);
		//	i_err = pngSeq->init(param);
  //          if (i_err != HJ_OK)
  //          {
		//		HJFLoge("HJRenderFaceuBaseMgr::draw() pngSeq->init() failed");
  //              break;
  //          }
		//	pngSeq->adjustResolution(i_width, i_height);
		//}
		//HJBaseParam::Ptr param = HJBaseParam::Create();
		//i_err = pngSeq->update(param);
		//if ((i_err == HJ_OK) && pngSeq->IsStateReady())
		//{
		//	static HJOGBaseShader::Ptr baseShader = nullptr;
		//	if (!baseShader)
		//	{
		//		baseShader = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
		//	}

		//	//GLuint textureId, int i_mode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false)
		//	baseShader->draw(pngSeq->getTextureId(), HJWindowRenderMode_FIT, pngSeq->getWidth(), pngSeq->getHeight(), m_fboCtrl->getWidth(), m_fboCtrl->getHeight(), pngSeq->getTexMatrix());

		//	m_pbReadWrapper->process(i_width, i_height);
		//}