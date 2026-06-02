#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"
#include "HJTransferInfo.h"
#include "HJRteUtils.h"
#include <memory>
#include "HJMediaUtils.h"
#include "HJOGUtils.h"
#include "HJFboLease.h"
#include "HJOGCommon.h"

NS_HJ_BEGIN

class HJRteCom;
class HJOGBaseShader;

class HJRteDriftInfo : public HJBaseParam
{
public:
    HJ_DEFINE_CREATE(HJRteDriftInfo);
    HJRteDriftInfo() = default;
    virtual ~HJRteDriftInfo();
    
 
    HJRteDriftInfo(const std::string& i_strComName, GLuint i_textureId, HJRteTextureType i_textureType, int i_width, int i_height, float *i_texMatrix = nullptr):
        m_strComName(i_strComName)
        ,m_textureId(i_textureId)
        ,m_textureType(i_textureType)
        ,m_srcWidth(i_width)
        ,m_srcHeight(i_height)     
    {
        setTextureMat(i_texMatrix);
    }
    
    //fixme after ref count, FBO cache, can memory;
    GLuint m_textureId = 0;
    
    void setVertexMat(float *i_mat)
    {
        if (i_mat)
        {
            memcpy(m_vertexMat, i_mat, sizeof(m_vertexMat));
        }
    }
    void setTextureMat(float *i_mat)
    {
        if (i_mat)
        {
            memcpy(m_textureMat, i_mat, sizeof(m_textureMat));
        }
    }

    const std::string& getSrcComName() const
    {
        return m_strComName;
    }  

    void attachFboLease(const std::shared_ptr<HJFboLease>& i_lease)
    {
        m_fboLease = i_lease;
    }
    //std::shared_ptr<HJFboLease> getFboLease() const
    //{
    //    return m_fboLease;
    //}
    //bool hasFboLease() const
    //{
    //    return (m_fboLease != nullptr);
    //}

    std::string m_strComName = "";
    HJRteTextureType m_textureType = HJRteTextureType_2D;
    int m_srcWidth = 0;
    int m_srcHeight = 0;
    float m_vertexMat[16]  = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
    float m_textureMat[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
    std::shared_ptr<class HJFboLease> m_fboLease = nullptr;
};

class HJRteComLinkInfo : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteComLinkInfo);
    HJRteComLinkInfo();
    HJRteComLinkInfo(float i_x, float i_y, float i_width, float i_height, HJWindowRenderMode i_renderMode = HJWindowRenderMode_CLIP, bool i_xMirror = false, bool i_yMirror = false, const std::string& i_linkId = "");
    virtual ~HJRteComLinkInfo();

    std::string getInfo() const;
    
    HJVec4i convert(int i_targetWidth, int i_targetHeight);
        
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 1.0f;
    float m_height = 1.0f;
    HJWindowRenderMode m_renderMode = HJWindowRenderMode_CLIP;
    bool m_xMirror = false;
    bool m_yMirror = false;
    std::string m_linkId = "";
};

class HJRteComLink : public HJBaseSharedObject
{
public:
	HJ_DEFINE_CREATE(HJRteComLink);
    HJRteComLink();
    virtual ~HJRteComLink();
    HJRteComLink(const std::shared_ptr<HJRteCom>& i_srcCom, const std::shared_ptr<HJRteCom>& i_dstCom, std::shared_ptr<HJRteComLinkInfo> i_linkInfo = nullptr, std::shared_ptr<HJOGBaseShader> i_shader = nullptr);

    void setReady(bool i_ready);
    bool isReady() const;

    std::shared_ptr<HJRteCom> getSrcComPtr() const;
    std::shared_ptr<HJRteCom> getDstComPtr() const;
    std::string getSrcComName() const;
    std::string getDstComName() const;
    std::string getInfo() const;
    const std::shared_ptr<HJRteComLinkInfo>& getLinkInfo() const
    {
        return m_linkInfo;
    }
    const std::shared_ptr<HJOGBaseShader>& getShader() const
    {
        return m_shader;
    }
    void setLinkInfo(std::shared_ptr<HJRteComLinkInfo> i_linkInfo)
    {
        if (i_linkInfo)
        {
            m_linkInfo = std::move(i_linkInfo);
        }
    }
    int breakLink();

    void setDrawEnable(bool i_enable)
    {
        m_bDrawEnable = i_enable;
    }   
    bool isDrawEnable() const
    {
        return m_bDrawEnable;
    }
private:
    void priMemoryStat();
    static std::atomic<int> m_memoryRteComLinkStatIdx;

    std::shared_ptr<HJRteComLinkInfo> m_linkInfo = nullptr;
    std::shared_ptr<HJOGBaseShader> m_shader = nullptr;
    bool m_bReady = false;
    std::weak_ptr<HJRteCom> m_srcCom;
    std::weak_ptr<HJRteCom> m_dstCom;
    bool m_bDrawEnable = true;
};

class HJRteCom : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteCom);

    HJRteCom();
    virtual ~HJRteCom();

    virtual int init(HJBaseParam::Ptr i_param);
    virtual void done();
    
    virtual void reset();
    virtual int setParam(HJBaseParam::Ptr i_param)
    {
        return HJ_OK;
    }
    

    virtual int bind()
    {
        return HJ_OK;
    }
    virtual int unbind(bool i_bDraw)
    {
        return HJ_OK;
    }
    virtual int render(const std::shared_ptr<HJRteComLink>&i_link, const HJRteDriftInfo::Ptr& i_drift)
    {
        return HJ_OK;
    }

    virtual int getWidth()
    {
        return 0;
    }
    virtual int getHeight()
    {
        return 0;
    }
    virtual float *getTexMatrix()
    {
        return HJOGCommon::IdentifyMatrix;
    }
    virtual GLuint getTextureId()
    {
        return 0;
    }
    virtual HJRteTextureType getTextureType() 
    {
        return HJRteTextureType_2D;
    }
    virtual int adjustResolution(int i_width, int i_height)
    {
        return 0; 
    }
//    virtual int update(HJBaseParam::Ptr i_param);
//    virtual int render(HJBaseParam::Ptr i_param);

    HJRteComLink::Ptr addTarget(HJRteCom::Ptr i_com, std::shared_ptr<HJOGBaseShader> i_shader = nullptr, std::shared_ptr<HJRteComLinkInfo> i_linkInfo = nullptr);

    void setUsePriority(bool i_use)
    {
        m_bUsePriority = i_use;
    }
    bool isUsePriority() const
    {
        return m_bUsePriority;
    }
    int getPriority() const
    {
        return (int)m_priority;
    }

    void setPriority(HJRteComPriority i_priority)
    {
        m_priority = i_priority;
    }
    int getIndex() const
    {
        return m_curIdx;
    }
    void setNotify(HJBaseNotify i_notify)
    {
        m_notify = i_notify;
    }
    
    bool isAllPreReady();

    void removePre(const std::shared_ptr<HJRteComLink>& i_link);
    void removeNext(const std::shared_ptr<HJRteComLink>& i_link);
    bool isPreEmpty();
    bool isNextEmpty();
    int getPreCount();
    int getNextCount();
    HJRteComType getRteComType() const;
    void setRteComType(HJRteComType i_type);
    bool isSource() const;
    bool isFilter() const;
    bool isTarget() const;
    //bool renderModeIsContain(int i_targetType);
    //void renderModeClear(int i_targetType);
    //void renderModeClearAll();
    //std::vector<HJTransferRenderModeInfo::Ptr>& renderModeGet(int i_targetType);
    //void renderModeAdd(int i_targetType, HJTransferRenderModeInfo::Ptr i_renderMode = nullptr);
    
    const std::deque<HJRteComLink::Wtr>& getNextQueue()
    {
        return m_nextQueue;
    }
	const std::deque<HJRteComLink::Wtr>& getPreQueue()
	{
		return m_preQueue;
	}
    void clearNext()
    {
        m_nextQueue.clear();
    }
    void clearPre()
    {
        m_preQueue.clear();
    }
    //void releaseFromPre(const std::shared_ptr<HJRteCom>& i_link);

    void setEnable(bool i_enable)
    {
        m_bEnable = i_enable;
    }
    bool isEnable() const
    {
        return m_bEnable;
    }

    int foreachNextLink(std::function<int(const std::shared_ptr<HJRteComLink>& i_link)> i_func);
    int foreachPreLink(std::function<int(const std::shared_ptr<HJRteComLink>& i_link)> i_func);

	HJRteDriftInfo::Ptr getDriftInfo();
	void setDriftInfo(HJRteDriftInfo::Ptr i_driftInfo);

    void refCntClear();
    int  refCntGet() const;
    void refCntSet(int i_refCnt);
protected:
    HJBaseNotify m_notify = nullptr;
    std::deque<HJRteComLink::Wtr> m_nextQueue;
    std::deque<HJRteComLink::Wtr> m_preQueue;
    
private:
    HJRteDriftInfo::Ptr m_driftInfo = nullptr;
    static bool pricompare(const std::weak_ptr<HJRteComLink>& i_a, const std::weak_ptr<HJRteComLink>& i_b);
    static std::atomic<int> m_memoryRteComStatIdx;
    
    int m_curIdx = 0;
    HJRteComPriority m_priority = HJRteComPriority_Default;
    bool m_bUsePriority = true;
    bool m_bEnable = true;

    int m_refCntStable = 0;
    int m_refCntEvery = 0;

    HJRteComType m_rteComType = HJRteComType_Filter;
    

    //int m_curIdx = 0;
    //HJPrioComType m_priority = HJPrioComType_None;
    //std::unordered_map<int, std::vector<HJTransferRenderModeInfo::Ptr>> m_renderMap;
};



NS_HJ_END



