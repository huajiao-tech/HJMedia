#pragma once

#include "HJPrerequisites.h"
#include "HJMapTab.h"
#include "HJThreadPool.h"

NS_HJ_BEGIN

#define HJ_CatchName(A) #A
#define HJ_SetInsName(A) setInsName(typeid(A).name());

class HJSPBuffer;
class HJBaseObject;
class HJBaseCom;
class HJBaseNotifyInfo;

using HJFunc = std::function<int()>;
using HJBaseNotify = std::function<void(int i_nType, const HJBaseNotifyInfo& i_info)>;
using HJBaseComWtrQueue = std::deque<std::weak_ptr<HJBaseCom>>;
using HJBaseComPtrQueue = std::deque<std::shared_ptr<HJBaseCom>>;
using HJThreadWtrQueue = std::deque<std::weak_ptr<HJThreadPool>>;

using HOVideoSurfaceCb = std::function<int(void *i_window, int i_width, int i_height, bool i_bCreate)>;

class HJBaseObject
{
public:
	HJ_DEFINE_CREATE(HJBaseObject);
	HJBaseObject();
	virtual ~HJBaseObject();
	virtual void setInsName(const std::string& i_insName);
    const std::string& getInsName() const;
protected:
	std::string m_insName = "";
};

typedef enum HJComFilterType
{
    HJCOM_FILTER_TYPE_NONE     = 0,
	HJCOM_FILTER_TYPE_VIDEO    = 1 << 1,
	HJCOM_FILTER_TYPE_AUDIO    = 1 << 2,
	HJCOM_FILTER_TYPE_SUBTITLE = 1 << 3,
} HJComFilterType;

class HJBaseParam : public HJMapTab
{
public:
	HJ_DEFINE_CREATE(HJBaseParam);
	HJBaseParam();
	virtual ~HJBaseParam();
    
    static std::string s_paramFps;
    static std::string s_paramRenderBridge;
};  

class HJBaseNotifyInfo : public HJMapTab
{
public:
	HJ_DEFINE_CREATE(HJBaseNotifyInfo);
	HJBaseNotifyInfo()
    {
        
    }
	virtual ~HJBaseNotifyInfo()
    {
        
    }
};  

class HJComMedaInfo : public HJMapTab
{
public:
	HJ_DEFINE_CREATE(HJComMedaInfo);
	HJComMedaInfo();
	virtual ~HJComMedaInfo();
};

typedef enum HJComMediaFrameInfo
{
	MEDIA_UNKNOWN = 0,

	MEDIA_VIDEO       = 1 << 1,
	MEDIA_AUDIO       = 1 << 2,

	MEDIA_DATA_HEADER = 1 << 3,
	MEDIA_DATA_KEY    = 1 << 4,
	MEDIA_DATA_NORMAL = 1 << 5,

	MEDIA_VIDEO_H264  = 1 << 6,
	MEDIA_VIDEO_H265  = 1 << 7,

} HJComMediaFrameInfo;

class HJComMediaFrame : public HJMapTab
{
public:
    HJ_DEFINE_CREATE(HJComMediaFrame);
    HJComMediaFrame();
    virtual ~HJComMediaFrame();

    bool IsVideo() const { return m_nFlag & MEDIA_VIDEO; }
    bool IsAudio() const { return m_nFlag & MEDIA_AUDIO; }

    bool IsVideoH265() const { return m_nFlag & MEDIA_VIDEO_H265; }
    bool IsVideoH264() const { return m_nFlag & MEDIA_VIDEO_H264; }

    bool IsDataHeader() const { return m_nFlag & MEDIA_DATA_HEADER; }
    bool IsDataKey() const { return m_nFlag & MEDIA_DATA_KEY; }
    bool IsDataNormal() const { return m_nFlag & MEDIA_DATA_NORMAL; }

    void SetFlag(int flag) { m_nFlag |= flag; }
    int GetFlag() const { return m_nFlag; }
    void ClearFlag(int flag) { m_nFlag &= ~flag; }

	std::shared_ptr<HJSPBuffer> GetBuffer() const;
	void SetBuffer(std::shared_ptr<HJSPBuffer> buffer);

    int64_t GetPts() const { return m_pts; }
    void SetPts(int64_t pts) { m_pts = pts; }

    int64_t GetDts() const { return m_dts; }
    void SetDts(int64_t dts) { m_dts = dts; }
private:
	int m_nFlag = 0;
	std::shared_ptr<HJSPBuffer> m_buffer = nullptr;
	int64_t m_pts = 0;
	int64_t m_dts = 0;
};

NS_HJ_END



