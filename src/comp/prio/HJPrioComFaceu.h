#pragma once

#include "HJFacePointMgr.h"
#include "HJPrioCom.h"
#include "HJAsyncCache.h"
#include "HJMediaData.h"
#include "HJSlidingWinAvg.h"
#if defined(HarmonyOS)
    #include <GLES3/gl3.h>
#else
    typedef unsigned int GLuint;
#endif

NS_HJ_BEGIN

class HJOGCopyShaderStrip;
class HJOGPointShader;
class HJOGFBOCtrl;
class HJFaceuInfo;
class HJFacePointsReal;

class HJPrioComFaceu : public HJPrioCom
{
public:
    HJ_DEFINE_CREATE(HJPrioComFaceu);
    HJPrioComFaceu();
    virtual ~HJPrioComFaceu();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;
    
   // virtual int sendMessage(HJBaseMessage::Ptr i_msg) override;
    
    void setPointDebug(bool i_bDebug)
    {
        m_bUsePointDebug = i_bDebug;
    }

    HJFaceSubscribeFuncPtr getFaceSubscribePtr();
    
protected:

private:
    HJFaceSubscribeFuncPtr m_faceSubscribePtr = nullptr;
    std::shared_ptr<HJOGPointShader> m_pointShader = nullptr;
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
    std::shared_ptr<HJOGFBOCtrl> m_fbo = nullptr;	
    int m_sourceWidth = 0;
    int m_sourceHeight = 0;
    bool m_bDraw = false;
    
    std::shared_ptr<HJFacePointsReal> m_point = nullptr;
    std::shared_ptr<HJFaceuInfo> m_faceuInfo = nullptr;
    bool m_bUsePointDebug = false;
 };

NS_HJ_END



