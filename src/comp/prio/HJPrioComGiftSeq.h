#pragma once

#include "HJPrioCom.h"
#include "HJThreadPool.h"
#include "HJSPBuffer.h"
#include "HJAsyncCache.h"
#include "HJJsonBase.h"
#include "HJImgSeqInfo.h"



NS_HJ_BEGIN

class HJOGCopyShaderStrip;
class HJPrioComGiftSeq : public HJPrioCom
{
public:
    HJ_DEFINE_CREATE(HJPrioComGiftSeq);
    HJPrioComGiftSeq();
    virtual ~HJPrioComGiftSeq();

	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;           
private:
    HJImgSeqParse::Ptr m_parse = nullptr; 
    float m_matrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; 
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
};

NS_HJ_END



