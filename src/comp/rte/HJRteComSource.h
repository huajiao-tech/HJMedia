#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"
#include <queue>
#include <vector>
#include "HJTransferInfo.h"
#include "HJRteCom.h"

NS_HJ_BEGIN


class HJRteComSource : public HJRteCom
{
public:
    HJ_DEFINE_CREATE(HJRteComSource);
    HJRteComSource();
    virtual ~HJRteComSource();
    
    virtual bool isRenderReady();
    virtual int update(HJBaseParam::Ptr i_param);

    void setUseFilter(bool i_bUseFilter) { m_bUseFilter = i_bUseFilter; }
    bool getUseFilter() const { return m_bUseFilter; }

private:
    bool m_bUseFilter = false;
    //virtual int render(HJBaseParam::Ptr i_param) override;
};

class HJRteComSourceBridge2 : public HJRteComSource
{
public:
    HJ_DEFINE_CREATE(HJRteComSourceBridge2);
    HJRteComSourceBridge2();
    virtual ~HJRteComSourceBridge2();

    virtual bool isRenderReady() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    //virtual int render(HJBaseParam::Ptr i_param) override;
};

class HJRteComSourcePngSeq : public HJRteComSource
{
public:
    HJ_DEFINE_CREATE(HJRteComSourcePngSeq);
    HJRteComSourcePngSeq();
    virtual ~HJRteComSourcePngSeq();

    virtual bool isRenderReady() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    //virtual int render(HJBaseParam::Ptr i_param) override;
};



NS_HJ_END



