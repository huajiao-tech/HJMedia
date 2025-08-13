#pragma once

#include "HJComObject.h"
#include "HJDataDequeue.h"

NS_HJ_BEGIN

class HJComInterleave : public HJBaseComSync
{
public:
    HJ_DEFINE_CREATE(HJComInterleave);
    HJComInterleave();
    virtual ~HJComInterleave();
    
    virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    
    HJComMediaFrame::Ptr drain();
    
private:
    int priTryDrainDetail(HJDataDequeNoLock<HJComMediaFrame::Ptr> &i_refDeque, int64_t i_curTime);
    int priTryDrain();
    std::mutex m_inter_mutex;
    HJDataDequeNoLock<HJComMediaFrame::Ptr> m_videoQueue;
    HJDataDequeNoLock<HJComMediaFrame::Ptr> m_audioQueue;
    bool m_bStarted = false;
   //int64_t m_startTime = 0;
    int64_t m_firstVideoTime = INT64_MIN;
    int64_t m_firstAudioTime = INT64_MIN;
};

NS_HJ_END



