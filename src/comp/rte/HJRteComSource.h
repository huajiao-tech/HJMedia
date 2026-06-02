#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"
#include <queue>
#include <vector>
#include "HJTransferInfo.h"
#include "HJRteCom.h"
#include "HJFacePointMgr.h"
//#include "HJMediaInfo.h"
#include "HJTransferMediaData.h"
#if defined (HarmonyOS)
    #include <GLES3/gl3.h>
#else
    typedef unsigned int GLuint;
#endif

NS_HJ_BEGIN
class HJRteComSource : public HJRteCom
{
public:
    HJ_DEFINE_CREATE(HJRteComSource);
    HJRteComSource() = default;
    virtual ~HJRteComSource() = default;
    
    virtual int init(HJBaseParam::Ptr i_param) override
    {
        return HJRteCom::init(i_param);
    }
    virtual void done() override;
       
    virtual int update(HJBaseParam::Ptr i_param)
    {
        return HJ_OK;
    }
    
    virtual int getWidth() override
    {
        return m_width;
    }
    virtual int getHeight() override
    {
        return m_height;
    }
    virtual float *getTexMatrix() override
    {
        return HJRteCom::getTexMatrix();
    }
    virtual GLuint getTextureId() override
    {
        return 0;
    }
    virtual HJRteTextureType getTextureType() override
    {
        return m_textureType;
    }

    virtual bool IsStateReady()
    {
        return true;
    }
    virtual int adjustResolution(int i_width, int i_height) override
    {
        m_width = i_width;
        m_height = i_height;
        return 0;
    }

    void setMainSource(bool i_bMainSource) { m_bMainSource = i_bMainSource; }
    bool isMainSource() const { return m_bMainSource; }
    void setDependsOn(const std::string& i_dependsOn) { m_dependsOn = i_dependsOn; }
    const std::string& getDependsOn() const { return m_dependsOn; }

    // void setWidth(int i_width) { m_width = i_width; }
    // void setHeight(int i_height) { m_height = i_height; }
    std::shared_ptr<HJOGFBOCtrl> takeFbo()
    {
		std::shared_ptr<HJOGFBOCtrl> ret = nullptr;
		if (m_dynamicFbo)
		{
			ret = std::move(m_dynamicFbo);
            m_dynamicFbo = nullptr;
		}
		return ret;
    }

protected:
    int m_width = 0;
    int m_height = 0;
    HJRteTextureType m_textureType = HJRteTextureType_2D;
    std::shared_ptr<HJOGFBOCtrl> m_dynamicFbo = nullptr;
private:
    bool m_bMainSource = false;
    std::string m_dependsOn = "";
    
};

class HJOGRenderWindowBridge;
class HJRteComSourceBridge : public HJRteComSource
{
public:
    HJ_DEFINE_CREATE(HJRteComSourceBridge);
    HJRteComSourceBridge();
    virtual ~HJRteComSourceBridge();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
       
    virtual int update(HJBaseParam::Ptr i_param) override;
    
    virtual int getWidth() override;
    virtual int getHeight() override; 
    virtual float *getTexMatrix() override;
    virtual GLuint getTextureId() override;
    virtual bool IsStateReady() override;
    virtual bool IsStateAvaiable();

    void stat();

    const std::shared_ptr<HJOGRenderWindowBridge>& getBridge(); 
    static HJRteComSourceBridge::Ptr CreateFactory();
#if defined(HarmonyOS)
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquireSoft();
    void renderWindowBridgeReleaseSoft(); 
#endif

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

class HJRteComSourceBridgeMediaData : public HJRteComSourceBridge
{
public:
	HJ_DEFINE_CREATE(HJRteComSourceBridgeMediaData);
    HJRteComSourceBridgeMediaData();
	virtual ~HJRteComSourceBridgeMediaData();

	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;

	virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int setParam(HJBaseParam::Ptr i_param) override;

	virtual int getWidth() override;
	virtual int getHeight() override;
	virtual float* getTexMatrix() override;
	virtual GLuint getTextureId() override;
    virtual bool IsStateReady() override;
    virtual bool IsStateAvaiable() override;
private:
    HJTransferMediaDataGetCb m_mediaDataGetCb = nullptr;
    HJSPBuffer::Ptr m_RGBABuf = nullptr;
    bool m_bCreateTexture = false;
    GLuint m_textureId = 0;
    int m_catchWidth = 0;
    int m_catchHeight = 0;
    bool m_bReady = false;
};

class HJRteComSourceSplitScreen : public HJRteComSourceBridge
{ 
public:
    HJ_DEFINE_CREATE(HJRteComSourceSplitScreen);
    HJRteComSourceSplitScreen();
    virtual ~HJRteComSourceSplitScreen();

    virtual int getWidth() override;
};

class HJRteComSourceSplitScreenMediaData : public HJRteComSourceBridgeMediaData
{
public:
	HJ_DEFINE_CREATE(HJRteComSourceSplitScreenMediaData);
    HJRteComSourceSplitScreenMediaData();
	virtual ~HJRteComSourceSplitScreenMediaData() = default;

    virtual int getWidth() override;
};

class HJOGCopyShaderStrip;
class HJOGPointShader;
class HJOGFBOCtrl;
class HJFaceuInfo;
class HJFacePointsReal;
class HJRteComSourceFaceu : public HJRteComSource
{ 
public:
    HJ_DEFINE_CREATE(HJRteComSourceFaceu);
    HJRteComSourceFaceu();
    virtual ~HJRteComSourceFaceu();
    
    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
       
    virtual int update(HJBaseParam::Ptr i_param) override;   
    virtual int getWidth() override;
    virtual int getHeight() override;
    virtual GLuint getTextureId() override;
    virtual bool IsStateReady() override;
    //virtual int adjustResolution(int i_width, int i_height) override;
    //void setFakePoints();
    int resetFaceu(HJBaseParam::Ptr i_param);
    //HJFaceSubscribeFuncPtr getFaceSubscribePtr();

    std::shared_ptr<HJFaceuInfo> getFaceuInfo();
    const std::string& getFaceInfoSource() const { return m_faceInfoSource; }

private:
    std::shared_ptr<HJMoreFacePointsReal> priGetMoreFacePoints();
    int priTryOpen(HJBaseParam::Ptr i_param);
    int priDraw(std::shared_ptr<HJMoreFacePointsReal> i_morePoints);
    std::shared_ptr<HJOGPointShader> m_pointShader = nullptr;
    bool m_bReady = false;
    
    //std::shared_ptr<HJFacePointsReal> m_point = nullptr;

    std::shared_ptr<HJMoreFacePointsReal> m_morePoints = nullptr;

    std::shared_ptr<HJFaceuInfo> m_faceuInfo = nullptr;
    FacePointAcquireFunc m_facePointAcquireFunc = nullptr;
    MoreFacePointAcquireFunc m_moreFacePointAcquireFunc = nullptr;
    std::string m_faceInfoSource = "";
    //HJFaceSubscribeFuncPtr m_faceSubscribePtr = nullptr;
    bool m_bUseFBO = true;
};

class HJImgSeqParse;
class HJRteComSourceImgSeq : public HJRteComSource
{
public:
    HJ_DEFINE_CREATE(HJRteComSourceImgSeq);
    HJRteComSourceImgSeq();
    virtual ~HJRteComSourceImgSeq();

    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
       
    virtual int update(HJBaseParam::Ptr i_param) override;   
    virtual int getWidth() override;
    virtual int getHeight() override;
    virtual GLuint getTextureId() override;
    virtual bool IsStateReady() override;
    virtual float* getTexMatrix() override;
    HJRectf getRect();
private:
    std::shared_ptr<HJImgSeqParse> m_parse = nullptr;
};

class HJRteComSourceImage : public HJRteComSource
{
public:
    HJ_DEFINE_CREATE(HJRteComSourceImage);
    HJRteComSourceImage();
    virtual ~HJRteComSourceImage();

    virtual int init(HJBaseParam::Ptr i_param) override;
    virtual void done() override;

    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int getWidth() override;
    virtual int getHeight() override;
    virtual GLuint getTextureId() override;
    virtual bool IsStateReady() override;
    virtual float* getTexMatrix() override;
    //HJRectf getRect();
private:
    bool m_bTextureReady = false;
    GLuint m_texture = 0;
    //HJRectf m_rect{0.f, 0.f, 1.f, 1.f};
};

NS_HJ_END



