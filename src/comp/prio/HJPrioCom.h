#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"
#include <queue>
#include <vector>
#include "HJTransferInfo.h"

NS_HJ_BEGIN

typedef enum HJPrioComType
{
    HJPrioComType_None = 0,
    HJPrioComType_VideoDefault   = 1,
    HJPrioComType_VideoBridgeSrc = 10,
    HJPrioComType_VideoBeauty    = 50,
    HJPrioComType_VideoGray      = 60,
    HJPrioComType_VideoBlur      = 100,

    HJPrioComType_FaceU2D      = 150,
    HJPrioComType_GiftSeq2D    = 200,
    HJPrioComType_GiftVideo2D  = 201,
} HJPrioComType;

class HJPrioCom : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJPrioCom);

    HJPrioCom();
    virtual ~HJPrioCom();

    virtual int init(HJBaseParam::Ptr i_param);
    virtual void done();
    
    virtual int update(HJBaseParam::Ptr i_param);
    virtual int render(HJBaseParam::Ptr i_param);
    
    int getPriority() const 
    {
        return (int)m_priority; 
    }
    void setPriority(HJPrioComType i_priority) 
    { 
        m_priority = i_priority; 
    }
    int getIndex() const 
    { 
        return m_curIdx;
    }
    void setNotify(HJBaseNotify i_notify)
    {
        m_notify = i_notify;
    }
    bool renderModeIsContain(int i_targetType);
    void renderModeClear(int i_targetType);
    void renderModeClearAll();
    std::vector<HJTransferRenderModeInfo::Ptr>& renderModeGet(int i_targetType);
    void renderModeAdd(int i_targetType, HJTransferRenderModeInfo::Ptr i_renderMode = nullptr);
    
protected:
    HJBaseNotify m_notify = nullptr;
private:
    static std::atomic<int> m_index;

    static std::atomic<int> m_memoryPrioComStatIdx;
    
    int m_curIdx = 0;
    HJPrioComType m_priority = HJPrioComType_None;
    std::unordered_map<int, std::vector<HJTransferRenderModeInfo::Ptr>> m_renderMap;
};



NS_HJ_END



