#pragma once

#include "HJPrerequisites.h"
#include "HJMapTab.h"

NS_HJ_BEGIN

class HJThreadPool;

#define HJ_CatchName(A) #A
#define HJ_SetInsName(A) setInsName(typeid(A).name());
#define HJ_CvtBaseToDerived(DerivedClass, basePtr) \
	DerivedClass::Ptr the = std::dynamic_pointer_cast<DerivedClass>(basePtr);\
	if(!the) \
	{ \
		i_err = -1;\
		break;\
	}\

#define HJ_CatchMapPlainSetVal(param, Type, name, value) (*param)[name] = (Type)value;
#define HJ_CatchMapPlainGetVal(param, Type, name, value) \
    if (param->contains(name)) \
		value = param->getValue<Type>(name);

#define HJ_CatchMapSetVal(param, Type, value)  HJ_CatchMapPlainSetVal(param, Type, HJ_CatchName(Type), value)
#define HJ_CatchMapGetVal(param, Type, value)  HJ_CatchMapPlainGetVal(param, Type, HJ_CatchName(Type), value)

#define HJ_CvtDynamic(T, obj)  std::dynamic_pointer_cast<T>(obj);

#define HJThreadFuncDef(thread)\
int asyncClear(HJThreadTaskFunc task, int i_id)\
{\
	return thread->asyncClear(task, i_id);\
}\
int async(HJThreadTaskFunc task, int64_t i_delayTime = 0)\
{\
	return thread->async(task, i_delayTime);\
}\
int sync(HJThreadTaskFunc task)\
{\
	return thread->sync(task);\
}\
void setTimeout(int64_t i_time_out)\
{\
	return thread->setTimeout(i_time_out);\
}\
void signal()\
{\
	return thread->signal();\
}\

class HJSPBuffer;
class HJBaseObject;
class HJBaseCom;
class HJBaseNotifyInfo;
class HJBaseParam;

using HJFunc = std::function<int()>;
using HJBaseNotify = std::function<void(std::shared_ptr<HJBaseNotifyInfo> i_info)>;
using HJBaseComWtrQueue = std::deque<std::weak_ptr<HJBaseCom>>;
using HJBaseComPtrQueue = std::deque<std::shared_ptr<HJBaseCom>>;
using HJThreadWtrQueue = std::deque<std::weak_ptr<HJThreadPool>>;

using HJRenderEnvUpdate = std::function<int(std::shared_ptr<HJBaseParam>)>;
using HJRenderEnvDraw   = std::function<int(std::shared_ptr<HJBaseParam>)>;

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
    
private:
    static std::atomic<int> m_memoryBaseObjStatIdx;
    int m_curIdx = 0;
};

class HJBaseSharedObject : public HJBaseObject, public std::enable_shared_from_this<HJBaseSharedObject>
{
public:
	HJ_DEFINE_CREATE(HJBaseObject);
	HJBaseSharedObject()
	{

	}
	virtual ~HJBaseSharedObject()
	{

	}
	std::shared_ptr<HJBaseSharedObject> getBaseShared()
	{
		return shared_from_this(); 
	}

	template<typename T>
	std::shared_ptr<T> getShared() 
	{
		return std::dynamic_pointer_cast<T>(shared_from_this());
	}

	template <typename T>
	std::shared_ptr<T> getSharedFrom(T* ptr)
	{
		return std::dynamic_pointer_cast<T>(ptr->shared_from_this());
	}
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
	int getType() const 
	{
		return m_nType; 
	}
	void setType(int i_type)
	{
		m_nType = i_type;
	}
	static HJBaseNotifyInfo::Ptr Create(int i_type)
	{
		HJBaseNotifyInfo::Ptr ptr = HJBaseNotifyInfo::Create();
		ptr->setType(i_type);
		return ptr;
	}
private:
	int m_nType = 0;
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



