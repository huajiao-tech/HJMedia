#pragma once

#include "HJPrioCom.h"
#include "HJPrioComSourceBridge.h"

#if defined(HarmonyOS)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif

NS_HJ_BEGIN

class HJPrioGraph;
class HJOGCopyShaderStrip;
class HJPrioComBaseFBOInfo;
class HJPrioComFBOBase;

class HJPrioComSourceSeries : public HJPrioComSourceBridge
{
public:
    HJ_DEFINE_CREATE(HJPrioComSourceSeries);
    HJPrioComSourceSeries();
    virtual ~HJPrioComSourceSeries();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
       
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;
    
    int openEffect(HJBaseParam::Ptr i_param);
    void closeEffect(HJBaseParam::Ptr i_param);
    
protected:
    
private:
    std::shared_ptr<HJPrioGraph> m_graph = nullptr;
    
#if defined (HarmonyOS)
    std::shared_ptr<HJPrioComBaseFBOInfo> m_LastFboInfo = nullptr;
#endif
    std::shared_ptr<HJPrioComFBOBase> m_fbo = nullptr;
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
    
    int m_insIdx = 0;
};

NS_HJ_END



