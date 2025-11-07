#pragma once

//#include "HJOGBaseShader.h"
#include "HJPrerequisites.h"
#include "HJJsonBase.h"
#include "HJComUtils.h"
#include "HJNotify.h"
#include "HJMediaData.h"

NS_HJ_BEGIN

class HJTimerThreadPool;
class HJOGRenderWindowBridge;
class HJOGCopyShaderStrip;

class HJFacePointRectInfo : public HJJsonBase
{
public:
	HJ_DEFINE_CREATE(HJFacePointRectInfo);
	virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
	{
		int i_err = HJ_OK;
		do
		{
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, left, top, width, height);
		} while (false);
		return i_err;
	}
	int left = 0;
	int top = 0;
    int width = 0;
    int height = 0;
};
class HJFacePointPoseInfo : public HJJsonBase
{
public:
	HJ_DEFINE_CREATE(HJFacePointPoseInfo);
	virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
	{
		int i_err = HJ_OK;
		do
		{
			HJ_JSON_BASE_DESERIAL(m_obj, i_obj, yaw, pitch, roll);
		} while (false);
		return i_err;
	}
    double yaw = 0.0;
    double pitch = 0.0;
    double roll = 0.0;
};

class HJFaceSinglePointItem : public HJJsonBase
{ 
public:
	HJ_DEFINE_CREATE(HJFaceSinglePointItem);
	virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
	{
		int i_err = HJ_OK;
		do
		{
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, x, y);
		} while (false);
		return i_err;
	}
    int x = 0;
    int y = 0;
};

class HJFacePointItem : public HJJsonBase
{
public:
    HJ_DEFINE_CREATE(HJFacePointItem);
	virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
	{
		int i_err = HJ_OK;
		do
		{
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, probability, block);
            HJ_JSON_SUB_DESERIAL(m_obj, i_obj, rect);
            HJ_JSON_SUB_DESERIAL(m_obj, i_obj, pose);
            HJ_JSON_ARRAY_OBJ_DESERIAL(m_obj, i_obj, points, HJFaceSinglePointItem);
		} while (false);
		return i_err;
	}

    double probability = 0.0;
    int block = -1;
    HJFacePointRectInfo rect;
    HJFacePointPoseInfo pose;
    std::vector<HJFaceSinglePointItem::Ptr> points;
};
class HJFacePointsInfo : public HJJsonBase
{
public:
	HJ_DEFINE_CREATE(HJFacePointsInfo);
	virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
	{
		int i_err = HJ_OK;
		do
		{
            HJ_JSON_ARRAY_OBJ_ANOMYMOUS_DESERIAL(m_obj, i_obj, pointItems, HJFacePointItem);
		} while (false);
		return i_err;
	}
    std::vector<HJFacePointItem::Ptr> pointItems;
};

////////////////////////////////////////////////////////////////////////
class HJFaceuTextureInfo : public HJJsonBase
{
public:
    HJ_DEFINE_CREATE(HJFaceuTextureInfo);
    HJFaceuTextureInfo() = default;
    virtual ~HJFaceuTextureInfo();

    virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, mframeCount, radius_Type, mradius, mid_Type, mid_x, mid_y, scale_Type, scale_ratio, anchor_offset_x, anchor_offset_y, asize_offset_x, asize_offset_y, mfaceCount, imageName);
        } while (false);
        return i_err;
    }
    //virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr);
    void update();
    int draw(std::vector<HJPointf> i_points, int width, int height);
    void setMaxCount()
    {
        m_bMaxCount = true; 
    }
    bool isMaxCount() const
    {
        return m_bMaxCount;
    }
    
    int mframeCount = 0;
    int radius_Type = 0;
    int mradius = 0;
    int mid_Type = 0;

    int scale_Type = 0;
    int scale_ratio = 0;
    int anchor_offset_x = 0;
    int anchor_offset_y = 0;
    int asize_offset_x = 0;
    int asize_offset_y = 0;
    int mfaceCount = 1;


    double mid_x = 0.0;
    double mid_y = 0.0;
    
    std::string imageName = "";




    //add every image absolute path
    std::vector<std::string> m_imagePaths;
    int m_procIdx = 0;    
    bool m_bMaxCount = false;
    int m_curLoopIdx = 0;
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
};

class HJFaceuInfo : public HJJsonBase
{
public:
    HJ_DEFINE_CREATE(HJFaceuInfo);
    HJFaceuInfo() = default;
    virtual ~HJFaceuInfo();
    
    virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
    {
        int i_err = HJ_OK;      
        do
        {
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, Name, ID, Type, loop, music, framerate);

            HJ_JSON_ARRAY_OBJ_DESERIAL(m_obj, i_obj, texture, HJFaceuTextureInfo);
            HJ_JSON_ARRAY_PLAIN_DESERIAL(m_obj, i_obj, restart);

        } while (false);
        return i_err;
    }

    int parse(HJBaseParam::Ptr i_param);
    
    //virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr);

    std::string Name = "";
    std::string ID = "";        
    int Type = 0;
    int loop = 1;
    std::string music = ""; 
    int framerate = 15;

    std::vector<int> restart;
    std::vector<HJFaceuTextureInfo::Ptr> texture;

private:
    void priDone();
    int priCreateImgPaths();
    bool m_bDone = false;
    std::string m_rootPath = "";
    std::shared_ptr<HJTimerThreadPool> m_threadTimer = nullptr;
    HJListener m_renderListener = nullptr;
};

NS_HJ_END



