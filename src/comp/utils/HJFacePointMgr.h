#pragma once

#if 0
#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJFaceuInfo.h"
#include "HJAsyncCache.h"
#include "HJSlidingWinAvg.h"
#include "HJAdaptiveExpMovAvg.h"
#include "HJEventBus.h"

NS_HJ_BEGIN

#define SMOOTH_SLIDINGWINDOW_FILTER 1

class HJPrioCom;
class HJFacePointsReal;

class HJFacePointsEx
{
public:
	HJ_DEFINE_CREATE(HJFacePointsEx);
    HJFacePointsEx() = default;
	virtual ~HJFacePointsEx() = default;    
    
    HJFacePointsEx(int w, int h, std::string i_faceInfo)
    {
        m_width = w;
        m_height = h;
        m_bContainsFace = !i_faceInfo.empty();
        if (m_bContainsFace)
        {
            m_faceInfo.deserial(i_faceInfo);
        }    
    }
    bool isContainFace()
    {
        return m_bContainsFace;
    }
    
    HJFacePointItem::Ptr getFaceItem()
    {
        return m_faceInfo.pointItems[0];
    }
    
    std::vector<HJPointf> getFacePoints()
    {
        std::vector<HJPointf> points;
        HJFacePointItem::Ptr item = m_faceInfo.pointItems[0];
        for (auto pt : item->points)
        {
            points.push_back(HJPointf{(float)pt->x, (float)pt->y});
        }
        return points;
    }
    
    std::vector<HJPointf> getRectPoints()
    {
        HJFacePointRectInfo rect = m_faceInfo.pointItems[0]->rect;
        
        std::vector<HJPointf> rectPoints;
        rectPoints.push_back(HJPointf{(float)(rect.left), (float)rect.top});
        rectPoints.push_back(HJPointf{(float)(rect.left + rect.width), (float)rect.top});
        rectPoints.push_back(HJPointf{(float)(rect.left), (float)(rect.top + rect.height)});
        rectPoints.push_back(HJPointf{(float)(rect.left + rect.width), (float)(rect.top + rect.height)});
        return rectPoints;
    }
        
    int m_width = 0;
    int m_height = 0;
    bool m_bContainsFace = false;
    HJFacePointsInfo m_faceInfo;
};

using HJFaceSubscribeFunc = std::function<void(std::shared_ptr<HJFacePointsReal>)>;
using HJFaceSubscribeFuncPtr = std::shared_ptr<HJFaceSubscribeFunc>;

class HJFacePointMgr : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJFacePointMgr);
    HJFacePointMgr() = default;
    virtual ~HJFacePointMgr() = default;

    void setFaceInfo(HJFacePointsEx::Ptr faceInfo);
//    int registerCom(std::shared_ptr<HJPrioCom> i_com);
//    void unregisterCom(std::shared_ptr<HJPrioCom> i_com);
    void done();
    int procPre();
    int procPost();
    void setFaceFindCb(HJFaceFindCb i_cb)
    {
        m_faceFindCb = i_cb;
    }

    //HJFaceSubscribeFuncPtr registSubscriber(HJFaceSubscribeFunc i_subscriber);
    void registSubscriber(HJFaceSubscribeFuncPtr i_subscriberPtr);
    void unRegistSubscriber(HJFaceSubscribeFuncPtr i_subscriberPtr);
private:
    
    void priResetFilter();
    int priProc(std::shared_ptr<HJFacePointsReal> i_point);
    //bool priIsContains(std::shared_ptr<HJPrioCom> i_com);
    HJFacePointsEx::Ptr m_catchPoint = nullptr;
    HJFaceFindCb m_faceFindCb = nullptr;
    int m_catchWidth = 0;
    int m_catchHeight = 0;
    HJAsyncCache<HJFacePointsEx::Ptr> m_facePoints;	
    //std::deque<std::weak_ptr<HJPrioCom>> m_deque;
    

    std::vector<HJSlidingWinAvgPointf::Ptr> m_filterVec;
    
    HJLandmarkSmoother::Ptr m_smoother = nullptr;
    HJLandmarkDistanceStat::Ptr m_distanceStat = nullptr;
    
    static float s_distanceThres;
    static int s_maxAverage;
    static int s_minAverage;
    static std::string s_subscribeName;
    
    HJEventBus<std::shared_ptr<HJFacePointsReal>> m_eventBus;
};

NS_HJ_END

#endif

