#pragma once
#include "HJDataDequeue.h"
#include "HJMediaData.h"
#include "HJAsyncCache.h"
#include "HJPrioCom.h"
#include "HJPrioUtils.h"
#include "HJNotify.h"
#if defined (HarmonyOS)
    #include <GLES3/gl3.h>
#endif

NS_HJ_BEGIN

class HJOGRenderWindowBridge;

class HJPrioComSourceBridge : public HJPrioCom
{
public:
    HJ_DEFINE_CREATE(HJPrioComSourceBridge);
    HJPrioComSourceBridge();
    virtual ~HJPrioComSourceBridge();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
       
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;
    
    virtual void openPBO(HJMediaDataReaderCb i_cb);
    virtual void closePBO();    
    
    void stat();
    bool IsStateReady();
    bool IsStateAvaiable();
    int getWidth();
    int getHeight(); 
    float *getTexMatrix();
#if defined (HarmonyOS)
    GLuint getTextureId();
#endif
    const std::shared_ptr<HJOGRenderWindowBridge>& getBridge(); 
    
    static HJPrioComSourceBridge::Ptr CreateFactory(HJPrioComSourceType i_type);
    
#if defined(HarmonyOS)
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquireSoft();
    void renderWindowBridgeReleaseSoft(); 
#endif
protected:
    HJListener m_renderListener = nullptr;
    
private:
    bool priIsMainReady();
    bool priIsSoftReady();
    bool priIsMainAvaiable();
    bool priIsSoftAvaiable();
    int priRender(const HJBaseParam::Ptr& i_param, const std::shared_ptr<HJOGRenderWindowBridge> & i_bridge);
    void priReleaseSoftBridge();
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJOGRenderWindowBridge> m_softBridge = nullptr;

    std::shared_ptr<HJOGRenderWindowBridge> m_nullBridge = nullptr;
    bool m_bUseSoftEnable = true;
    int m_statIdx = 0;
};

NS_HJ_END



