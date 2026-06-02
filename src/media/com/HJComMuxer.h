#pragma once

#include "HJComObject.h"
#include "HJRtmpMuxer.h"
#include "HJDataDequeue.h"
#include "HJRegularProc.h"

NS_HJ_BEGIN

class HJFFMuxer;

typedef enum HJComMuxerState
{
    HJComMuxerState_Idle = 0,
    HJComMuxerState_Ready,
    HJComMuxerState_Run,
} HJComMuxerState;

class HJComMuxer : public HJBaseComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJComMuxer);
    HJComMuxer();
    virtual ~HJComMuxer();
    
    virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    
protected:
	virtual int run() override ;    
private:
	HJSpDataDeque<HJComMediaFrame::Ptr> m_mediaFrameDeque;
    std::shared_ptr<HJFFMuxer> m_muxer = nullptr;
    
    int64_t m_firstVideoTime = INT64_MIN;
    int64_t m_firstAudioTime = INT64_MIN;
    int64_t m_startTime = 0;
    
    std::atomic<int> m_state{HJComMuxerState_Idle};
};

NS_HJ_END



