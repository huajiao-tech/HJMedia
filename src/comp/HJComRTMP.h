#pragma once

#include "HJComObject.h"
#include "HJRtmpMuxer.h"
#include "HJDataDequeue.h"
#include "HJRegularProc.h"

NS_HJ_BEGIN

class HJComRTMP : public HJBaseComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJComRTMP);
    HJComRTMP();
    virtual ~HJComRTMP();
    virtual void join() override;
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
protected:
	virtual int run() override ;    
private:
    void priStat(const HJComMediaFrame::Ptr& i_frame);
    HJRTMPMuxer::Ptr m_muxer = nullptr;
    
    std::mutex m_stat_mutex;
    HJRegularProcTimer m_regularLog;
    int64_t m_videoCacheDts = 0;
    int64_t m_audioCacheDts = 0;
    
    int64_t m_vvDiffDts = 0;
    int64_t m_avDiffDts = 0;
    int64_t m_aaDiffDts = 0;
};

NS_HJ_END



