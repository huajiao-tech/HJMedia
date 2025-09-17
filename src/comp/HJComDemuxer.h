#pragma once

#include "HJComObject.h"
#include "HJDataDequeue.h"
#include "HJRegularProc.h"
#include "HJComEvent.h"

NS_HJ_BEGIN

class HJBaseDemuxer;

class HJComDemuxer : public HJBaseComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJComDemuxer);
    HJComDemuxer();
    virtual ~HJComDemuxer();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual void join() override;
    
    void start();
    static int s_maxMediaSize;
protected:
	virtual int run() override;    
private:
    HJThreadPool::Ptr m_threadPool = nullptr;
    std::shared_ptr<HJBaseDemuxer> m_baseDemuxer = nullptr;

    std::atomic<bool> m_bStart{ false };
    bool m_bEof = false;
};

NS_HJ_END



