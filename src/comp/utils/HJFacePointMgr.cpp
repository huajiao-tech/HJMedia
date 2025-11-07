#include "HJFacePointMgr.h"
#include "HJFLog.h"
#include "HJPrioCom.h"

NS_HJ_BEGIN
    
float HJFacePointMgr::s_distanceThres = 6.f;
int HJFacePointMgr::s_maxAverage = 5;
int HJFacePointMgr::s_minAverage = 2;
std::string HJFacePointMgr::s_subscribeName = "faceEventBus";
void HJFacePointMgr::setFaceInfo(HJFacePointsEx::Ptr faceInfo)
{
    m_facePoints.enqueue(faceInfo);
}
//bool HJFacePointMgr::priIsContains(std::shared_ptr<HJPrioCom> i_com)
//{
//    bool bContais = false;
//    for (auto & wcom : m_deque)
//    {
//        std::shared_ptr<HJPrioCom> pcom = wcom.lock();
//        if (pcom && (pcom == i_com))
//        {
//            bContais = true;
//            break;
//        }    
//    }
//    return bContais;
//}
//int HJFacePointMgr::registerCom(std::shared_ptr<HJPrioCom> i_com)
//{
//    int i_err = HJ_OK;
//    do 
//    {
//        if (priIsContains(i_com))
//        {
//            break;
//        }    
//        m_deque.push_back(std::move(i_com));
//    } while (false);
//    return i_err;
//}
//void HJFacePointMgr::unregisterCom(std::shared_ptr<HJPrioCom> i_com)
//{
//    for (auto it = m_deque.begin(); it != m_deque.end(); it++)
//    {
//        std::shared_ptr<HJPrioCom> pcom = (*it).lock();
//        if (pcom && (pcom == i_com))
//        {
//            m_deque.erase(it);
//            break;
//        }    
//    }
//}
int HJFacePointMgr::priProc(HJFacePointsReal::Ptr i_point)
{
    int i_err = HJ_OK;
    do {

        m_eventBus.publish(s_subscribeName, i_point);

//        for (auto it = m_deque.begin(); it != m_deque.end();)
//        {
//            std::shared_ptr<HJPrioCom> pcom = (*it).lock();
//            if (!pcom)
//            {
//                HJFLogi("{} erase the empty object, weak pointer dynamic detect", getInsName());
//                it = m_deque.erase(it);
//            }    
//            else
//            {
//                HJBaseMessage::Ptr msg = HJBaseMessage::Create<HJBaseMessage>(HJCOM_MESSAGE_FACEPOINT);
//                HJ_CatchMapSetVal(msg, HJFacePointsReal::Ptr, i_point);
//                i_err = pcom->sendMessage(msg);
//                if (i_err < 0)
//                {
//                    HJFLoge("{} sendmessage error", getInsName());
//                    break;
//                }
//                it++;
//            }
//        }    
    } while (false);
    return i_err;
}
void HJFacePointMgr::priResetFilter()
{
#if SMOOTH_SLIDINGWINDOW_FILTER    
    for (auto f : m_filterVec)
    {
        f->reset();
    }    
#else
    if (m_smoother)
    {
        m_smoother->reset();
    }    
#endif
}
int HJFacePointMgr::procPre()
{
    int i_err = HJ_OK;
    do 
    {
        HJFacePointsEx::Ptr point = m_facePoints.acquire();
        if (!point)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }    
        m_catchPoint = point;
               
        if ((m_catchWidth != point->m_width) || (m_catchHeight != point->m_height))
        {
            m_catchWidth = point->m_width;
            m_catchHeight = point->m_height;
            priResetFilter();
        }
        bool bFindFace = point->isContainFace();
        if (m_faceFindCb)
        {
            m_faceFindCb(bFindFace);
        }
        HJFacePointsReal::Ptr facepoint = HJFacePointsReal::Create<HJFacePointsReal>(point->m_width, point->m_height, bFindFace);
        
        if (!m_distanceStat)
        {
            m_distanceStat = HJLandmarkDistanceStat::Create();
        }
        
        if (bFindFace)
        {
#if SMOOTH_SLIDINGWINDOW_FILTER
            int windowSize = s_maxAverage;
            float distance = m_distanceStat->stat(point->getFacePoints());
            if (distance < s_distanceThres)
            {
                //HJFLogi("facedistance small:{} < 5.f", distance); 
                windowSize = s_maxAverage;
            } 
            else if (distance >= s_distanceThres)
            {
                //HJFLogi("facedistance {}  >= {}", distance, s_distanceThres);
                windowSize = s_minAverage;
            }
            
            if (m_filterVec.empty())
            {
                for (int i = 0; i < point->getFacePoints().size(); i++)
                {
                    m_filterVec.push_back(std::move(HJSlidingWinAvgPointf::Create<HJSlidingWinAvgPointf>(s_maxAverage)));
                }
            }
            std::vector<HJPointf> pts = point->getFacePoints();
            for (int i = 0; i < m_filterVec.size(); i++)
            {
                m_filterVec[i]->updateWinSize(windowSize);
                facepoint->addFilterPoint(m_filterVec[i]->compute(pts[i]));
            }
            m_distanceStat->save(facepoint->getFilterPt());
            
            
            //add rect point, only debug;
            std::vector<HJPointf> rectPoints = point->getRectPoints();
            for (auto pt : rectPoints)
            {
                facepoint->addFilterPoint(pt);
            }
            
//            for (int i = 0; i < 1/*facepoint->getFilterPt().size()*/; i++)
//            {
//                HJFLogi("compare i:{} x y:{} {} xx yy {}:{}", i, pts[i].x, pts[i].y, facepoint->getFilterPt()[i].x, facepoint->getFilterPt()[i].y);    
//            }
#else
            if (!m_smoother)
            {
                m_smoother = HJLandmarkSmoother::Create();
            }    
            facepoint->copyFilterPoint(m_smoother->smooth(point->getFacePoints()));
#endif
        }    
        else 
        {
            //HJFLoge("facedistance no face"); 
            m_distanceStat->reset();
    
            priResetFilter();    
        } 
        
        i_err = priProc(facepoint);
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}
void HJFacePointMgr::done()
{
    HJFLogi("{} done enter", getInsName());
    HJFacePointsReal::Ptr emptyFacePoint = HJFacePointsReal::Create<HJFacePointsReal>(m_catchWidth, m_catchHeight, false);
    priProc(emptyFacePoint);
    m_facePoints.clear();
    //m_deque.clear();
    HJFLogi("{} done end", getInsName());
}
int HJFacePointMgr::procPost()
{
    if (m_catchPoint)
    {
        m_facePoints.recovery(m_catchPoint);
        m_catchPoint = nullptr;
    }
    return HJ_OK;
}

//HJFaceSubscribeFuncPtr HJFacePointMgr::registSubscriber(HJFaceSubscribeFunc i_subscriber)
//{
//	//std::shared_ptr<std::function<void(HJFacePointsReal::Ptr)>> v = m_eventBus.subscribe("", i_subscriber);
//    return m_eventBus.subscribe(s_subscribeName, i_subscriber);
//}
void HJFacePointMgr::registSubscriber(HJFaceSubscribeFuncPtr i_subscriberPtr)
{
    m_eventBus.subscribe(s_subscribeName, i_subscriberPtr);
}

NS_HJ_END