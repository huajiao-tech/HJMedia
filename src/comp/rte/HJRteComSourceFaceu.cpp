#include "HJRteComSource.h"
#include "HJFLog.h"
#include "HJFaceuInfo.h"
#include "HJFacePointMgr.h"
#include "HJOGBaseShader.h"
#include "HJOGPointShader.h"
#include "HJOGFBOCtrl.h"
#include "HJOGEGLSurface.h"
#include "HJRteGraphBaseEGL.h"
#include "HJFacePointsReal.h"

NS_HJ_BEGIN

HJRteComSourceFaceu::HJRteComSourceFaceu()
{
    HJ_SetInsName(HJRteComSourceFaceu);
    HJRteCom::setPriority(HJRteComPriority_FaceU2D);
    m_textureType = HJRteTextureType_2D;
}
HJRteComSourceFaceu::~HJRteComSourceFaceu()
{
    HJFLogi("{} ~HJRteComSourceFaceu", getDebugName());
}
//void HJRteComSourceFaceu::setFakePoints()
//{
 //   m_point = HJFacePointsReal::Create<HJFacePointsReal>(720, 1280, true);
	//m_point->add(HJPointf{ 343.f, 421.f });
	//m_point->add(HJPointf{ 435.f, 421.f });
	//m_point->add(HJPointf{ 397.f, 484.f });
	//m_point->add(HJPointf{ 349.f, 518.f });
	//m_point->add(HJPointf{ 422.f, 517.f });
	//m_point->add(HJPointf{ 262.f, 307.f });
	//m_point->add(HJPointf{ 483.f, 304.f });
	//m_point->add(HJPointf{ 262.f, 563.f });
	//m_point->add(HJPointf{ 483.f, 563.f });
//}

int HJRteComSourceFaceu::priTryOpen(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    { 
		std::string rootPath = "";
        if (i_param)
        {
            HJ_CatchMapPlainGetVal(i_param, std::string, HJRteUtils::ParamUrlFaceu, rootPath);
            HJ_CatchMapPlainGetVal(i_param, std::string, HJRteUtils::ParamFaceInfoSource, m_faceInfoSource);
        }
		if (!rootPath.empty())
		{
            if (i_param)
            {
                HJ_CatchMapGetVal(i_param, FacePointAcquireFunc, m_facePointAcquireFunc);
                HJ_CatchMapGetVal(i_param, MoreFacePointAcquireFunc, m_moreFacePointAcquireFunc);
            }
			m_faceuInfo = HJFaceuInfo::Create();
            m_faceuInfo->setInsName(getInsName());
			i_err = m_faceuInfo->parse(i_param);
			if (i_err < 0)
			{
				break;
			}
		}
    } while (false);    
    return i_err;
}

int HJRteComSourceFaceu::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteComSource::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        i_err = priTryOpen(i_param);
		if (i_err < 0)
		{
			break;
		}
        m_bUseFBO = HJRteGraphBaseEGL::getFBOCtrlPool() ? true : false;
        HJFLogi("{} init ok m_bUseFBO:{}", getInsName(), m_bUseFBO);
    } while (false);
    return i_err;
}
int HJRteComSourceFaceu::resetFaceu(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        if (i_param)
        {
			i_err = priTryOpen(i_param);
			if (i_err < 0)
			{
				break;
			}
        }
        else
        {
            m_faceuInfo = nullptr;
        }
    } while (false);
    return i_err;
}
void HJRteComSourceFaceu::done()
{
    m_faceuInfo = nullptr;
    HJRteComSource::done();
}
std::shared_ptr<HJMoreFacePointsReal> HJRteComSourceFaceu::priGetMoreFacePoints()
{
    std::shared_ptr<HJMoreFacePointsReal> morePoints = nullptr;

    HJFacePointsReal::Ptr pointCompatible = nullptr;
    if (m_facePointAcquireFunc)
    {
        pointCompatible = m_facePointAcquireFunc();
    }
    //harmony and m_facePointAcquireFunc; compatible with old version
    if (pointCompatible)
    {
        morePoints = HJMoreFacePointsReal::cvtFrom(pointCompatible);
  //      //only one face;
  //      morePoints = HJMoreFacePointsReal::Create<HJMoreFacePointsReal>(pointCompatible->width(), pointCompatible->height(), pointCompatible->isContainFace(), pointCompatible->getTimestamp());
  //      morePoints->setElapsedTime(pointCompatible->getElapsedTime());
  //      morePoints->setIsDebugPoints(pointCompatible->isDebugPoints());
		//morePoints->setSystemTime(pointCompatible->getSystemTime());
  //      
  //      if (pointCompatible->isContainFace())
  //      {
  //          HJSingleFacePointsReal::Ptr singlePoint = HJSingleFacePointsReal::Create();
  //          const std::vector<HJPointf>& ptVecor = pointCompatible->getFilterPt();
  //          //only first five points
  //          for (int i = 0; i < 5; i++)
  //          {
  //              singlePoint->add(ptVecor[i]);
  //          }
  //          //face rect
  //          singlePoint->setFaceRect(ptVecor[5].x, ptVecor[5].y, ptVecor[8].x - ptVecor[5].x, ptVecor[8].y - ptVecor[5].y);
  //          morePoints->add(singlePoint);
  //      }
    }
    else
    {
        if (m_moreFacePointAcquireFunc)
        {
            morePoints = m_moreFacePointAcquireFunc();
        }
    }
    return morePoints;
}
int HJRteComSourceFaceu::update(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        m_bReady = false;
        
        if (!m_faceuInfo)
        {
            break;
        }

        std::shared_ptr<HJMoreFacePointsReal> morePoints = priGetMoreFacePoints();
        if (!morePoints)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }

        i_err = m_faceuInfo->update();
        if ((i_err < 0) || (i_err == HJ_WOULD_BLOCK))
        {
            break;
        }

        if (m_bUseFBO)
        {
            if (!m_dynamicFbo)
            {
                m_dynamicFbo = HJRteGraphBaseEGL::getFBOCtrlPool()->acquire(getInsName(), m_width, m_height);
                if (!m_dynamicFbo)
                {
                    HJFLoge("{} get fbo failed width {} height {}", getInsName(), m_width, m_height);
                    break;
                }
            }
        }
        
		i_err = priDraw(morePoints);
        if (i_err < 0)
        {
            break;
        }
        
    } while (false);
    return i_err;
}
int HJRteComSourceFaceu::priDraw(std::shared_ptr<HJMoreFacePointsReal> i_morePoints)
{
    int i_err = HJ_OK;
    if (m_dynamicFbo)
    {
        m_dynamicFbo->attach();
    }
    do
    {
        if ((i_morePoints->width() == m_width) && (i_morePoints->height() == m_height))
        {
            // HJFLogi("Faceu face points wh<{} {}> p0:<{} {}> p3:<{} {}>", point->m_width, point->m_height, point->m_p0.x, point->m_p0.y, point->m_p3.x, point->m_p3.y);
            if (!i_morePoints->isContainFace())
            {
                break;
            }

            std::vector<HJSingleFacePointsReal::Ptr>&  vecPoints = i_morePoints->getPoints();
            for (auto& singlePoint : vecPoints)
            {
                i_err = m_faceuInfo->draw(singlePoint->getFilterPt(), m_width, m_height);
                if (i_err < 0)
                {
                    break;
                }
            }

            if (i_morePoints->isDebugPoints())
            {
                if (!m_pointShader)
                {
                    m_pointShader = HJOGPointShader::Create();
                    i_err = m_pointShader->init(9, 5.f);
                    if (i_err < 0)
                    {
                        break;
                    }
                }
                for (auto& singlePoint : vecPoints)
                {
                    std::vector<HJPointf> tmpPoints;
                    for (auto& pt : singlePoint->getFilterPt())
                    {
                        tmpPoints.push_back(HJOGPointShader::convert(pt, m_width, m_height));
                    }
                    //draw rect
                    const HJRectf& rect = singlePoint->getFaceRect();
                    tmpPoints.push_back(HJOGPointShader::convert(HJPointf{ (float)rect.x, (float)rect.y }, m_width, m_height));
					tmpPoints.push_back(HJOGPointShader::convert(HJPointf{ (float)(rect.x + rect.w), (float)rect.y }, m_width, m_height));
					tmpPoints.push_back(HJOGPointShader::convert(HJPointf{ (float)rect.x, (float)(rect.y + rect.h)}, m_width, m_height));
					tmpPoints.push_back(HJOGPointShader::convert(HJPointf{ (float)(rect.x + rect.w), (float)(rect.y + rect.h) }, m_width, m_height));

                    i_err = m_pointShader->update(tmpPoints);
                    if (i_err < 0)
                    {
                        break;
                    }
                    i_err = m_pointShader->draw();
                    if (i_err < 0)
                    {
                        break;
                    }
                }      
            }
            m_bReady = true;
        }
        else
        {
            HJFLogi("pricom face points {} {} not match {} {}", i_morePoints->width(), i_morePoints->height(), m_width, m_height);
        }
    } while (false);
    if (m_dynamicFbo)
    {
        m_dynamicFbo->detach();
    }
    return i_err;
}

int HJRteComSourceFaceu::getWidth()
{
    return m_width;
}
int HJRteComSourceFaceu::getHeight()
{
    return m_height;
}
GLuint HJRteComSourceFaceu::getTextureId()
{  
    if (!m_dynamicFbo)
    {
        return 0;
    }
    return m_dynamicFbo->getTextureId();
}
bool HJRteComSourceFaceu::IsStateReady()
{
    return m_bReady;
}

//HJFaceSubscribeFuncPtr HJRteComSourceFaceu::getFaceSubscribePtr()
//{
//    if (!m_faceSubscribePtr)
//    {
//        HJRteComSourceFaceu::Wtr wtr = HJBaseSharedObject::getSharedFrom(this);
//        m_faceSubscribePtr = std::make_shared<HJFaceSubscribeFunc>([wtr](HJFacePointsReal::Ptr i_point)
//        {
//            HJRteComSourceFaceu::Ptr ptr = wtr.lock();
//            if (ptr)
//            {
//                ptr->m_point = i_point;
//                //HJFLogi("{} face get publish result", ptr->getInsName());
//            }
//        });
//    }
//    return m_faceSubscribePtr;
//}
std::shared_ptr<HJFaceuInfo> HJRteComSourceFaceu::getFaceuInfo()
{
    return m_faceuInfo;
}

NS_HJ_END
