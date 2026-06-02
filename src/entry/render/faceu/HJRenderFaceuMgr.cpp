#include "HJRenderFaceuMgr.h"
#include "HJError.h"
#include "HJFLog.h"
#include "HJTime.h"
#include "HJOGFBOCtrl.h"
#include "HJRenderCoreSingleton.h"
#include "HJRteComSource.h"
#include "HJOGBaseShader.h"
#include "HJPBOReadWrapper.h"
#include "HJRenderFaceuObject.h"
#include "HJCoreVersion.h"

NS_HJ_BEGIN

HJRenderFaceuBaseMgr::HJRenderFaceuBaseMgr()
{
}
HJRenderFaceuBaseMgr::~HJRenderFaceuBaseMgr()
{
    HJFLogi("~HJRenderFaceuBaseMgr");
}

int HJRenderFaceuBaseMgr::init(HJFaceuListener i_listener)
{
	int i_err = HJ_OK;
	do
	{		
		HJFLogi("faceu manager init version:{}", HJCoreVersion::getVersionDetail());
		m_listener = i_listener;
	} while (false);
    return i_err;
}

int HJRenderFaceuBaseMgr::render(int i_width, int i_height, std::shared_ptr<HJMoreFacePointsReal> i_points, unsigned char*& o_pRGBA)
{
	int i_err = HJ_OK;
	do
	{
        int64_t t0 = HJCurrentSteadyMS();

		m_catchPoints = std::move(i_points);

		if (m_map.empty())
		{
			i_err = HJ_WOULD_BLOCK;
			break;
		}
		for (auto it = m_map.begin(); it!= m_map.end(); ++it)
		{
			int ret = it->second->render(i_width, i_height);
			if (ret < 0)
			{
				HJFLoge("HJRenderFaceuBaseMgr::render() it->second->render() failed");
			}
		}

        int64_t t1 = HJCurrentSteadyMS();
        m_statIdx++;
        int64_t diff = t1 - t0;
        m_statSum += diff;
        if (m_statIdx % 100 == 0)
        {
            HJFLogi("HJRenderFaceuBaseMgr::render() statIdx:{} curDiff:{} AvgTime: {}ms", m_statSum, diff, m_statSum / m_statSum);
        }

	} while (false);
    return i_err;
}
int HJRenderFaceuBaseMgr::add(const std::string& i_uniqueKey, const std::string& i_url, bool i_bDebugPoints)
{
    int i_err = HJ_OK;
	do
	{
		if (m_map.find(i_uniqueKey) != m_map.end())
		{
            HJFLoge("HJRenderFaceuBaseMgr::add() key:{} already exist", i_uniqueKey);
            break;
		}
		setDebugPoints(i_bDebugPoints);
		HJRenderFaceuObject::Ptr obj = HJRenderFaceuObject::Create();
		HJBaseParam::Ptr param = HJBaseParam::Create();

        HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlFaceu, i_url);
		HJ_CatchMapPlainSetVal(param, bool, "bEveryCompleteNotify", true);
		MoreFacePointAcquireFunc func = [this]()
		{
			return m_catchPoints;
		};
		HJ_CatchMapSetVal(param, MoreFacePointAcquireFunc, func);

		HJListener listener = [this, i_uniqueKey](const HJNotification::Ptr i_notification)
		{
			if (m_listener)
			{
				m_listener(i_uniqueKey, i_notification);
			}
			return HJ_OK;
		};
        HJ_CatchMapSetVal(param, HJListener, listener);
		
		i_err = obj->init(param);
        if (i_err != HJ_OK)
        {
            HJFLoge("HJRenderFaceuBaseMgr::add() HJRenderFaceuObject::init() failed");
            break;
        }
		m_map[i_uniqueKey] = obj;
	} while (false);
    return i_err;
}
int HJRenderFaceuBaseMgr::remove(const std::string& i_uniqueKey)
{
	int i_err = HJ_OK;
	do
	{	
		if (m_map.find(i_uniqueKey) != m_map.end())
		{
			HJFLogi("HJRenderFaceuBaseMgr::remove() key:{}", i_uniqueKey);
			m_map.erase(i_uniqueKey);
		}
	} while (false);
    return i_err;
}

int HJRenderFaceuBaseMgr::removeAll()
{
	int i_err = HJ_OK;
	do
	{
		m_map.clear();
	} while (false);
    return i_err;
}
void HJRenderFaceuBaseMgr::done()
{
	int i_err = HJ_OK;
	do
	{
		m_map.clear();
	} while (false);
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