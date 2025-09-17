#pragma once

#include "HJPrioCom.h"
#include "HJThreadPool.h"
#include "HJSPBuffer.h"
#include "HJAsyncCache.h"
#include "HJJsonBase.h"

#if defined(HarmonyOS)
    #include <GLES3/gl3.h>
#else
    typedef unsigned int GLuint;
#endif

NS_HJ_BEGIN

class HJOGCopyShaderStrip;

class HJPNGSeqConfigPosInfo : public HJJsonBase
{
 public:
    virtual int deserialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, topx, topy, width, height);
        } while (false);
        return i_err;
    }
    
    virtual int serialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_SERIAL(m_obj, i_obj, topx, topy, width, height);
        } while (false);
        return i_err;
    }
    
 	double topx = 0.0;
 	double topy = 0.0;
 	double width = 1.0;    
    double height = 1.0;  
};

class HJPNGSeqConfigInfo : public HJJsonBase
{
 public:
public:
    virtual int deserialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, prefix, loops, fps);
            HJ_JSON_SUB_DESERIAL(m_obj, i_obj, position);
        } while (false);
        return i_err;
    }
    
    virtual int serialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_SERIAL(m_obj, i_obj, prefix, loops, fps);
            HJ_JSON_SUB_SERIAL(m_obj, i_obj, position);
        } while (false);
        return i_err;
    }
 	
 	std::string prefix = "";
 	int loops = 1;
 	int fps = 30;
    
    HJPNGSeqConfigPosInfo position;
};

class HJRawImageDataInfo
{
public:
    HJ_DEFINE_CREATE(HJRawImageDataInfo);
    HJSPBuffer::Ptr m_buffer = nullptr;
    int m_width = 0;
    int m_height = 0;
    int m_components = 0;
};

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
protected:

private:
    int priCreateTexture();
    int priUpdate(HJRawImageDataInfo::Ptr i_rawInfo);
    float m_matrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; 
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
    HJTimerThreadPool::Ptr m_threadTimer = nullptr;
    std::vector<std::string> m_pngUrlQueue; 
    int m_pngIdx = 0;
    int m_pngLoopIdx = 0;
    bool m_bEnd = false;
    HJAsyncCache<HJRawImageDataInfo::Ptr> m_cache;
    
    bool m_bTextureReady = false;
    GLuint m_texture = 0;
    
    HJPNGSeqConfigInfo m_configInfo;
};

NS_HJ_END



