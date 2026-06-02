#pragma once

//#include "HJOGBaseShader.h"
#include "HJPrerequisites.h"
#include "HJJsonBase.h"
#include "HJComUtils.h"
#include "HJNotify.h"
#include "HJOGUtils.h"
#include "HJMediaData.h"
#include "HJSPBuffer.h"
#include "HJAsyncCache.h"

NS_HJ_BEGIN

class HJTimerThreadPool;
class HJOGRenderWindowBridge;
class HJOGCopyShaderStrip;

typedef enum HJFaceuStrategy
{
    HJFaceuStrategy_HarmonyBridge = 0,
    HJFaceuStrategy_SharedCtxTexture,
    HJFaceuStrategy_ExclusiveTexture
} HJFaceuStrategy;

class HJFaceuConstants
{
public:
	static const int AnchorAlignType_Eyebrows;
    static const int AnchorAlignType_Nose;
    static const int AnchorAlignType_Mouth;
    static const int AnchorAlignType_Fixed;

    static const int RotateType_Eyes;
    static const int RotateType_Fixed;

	static const int ScaleType_EyeDistance;
	static const int ScaleType_Fixed;
};



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
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, mframeCount, radius_Type, mradius, mid_Type, mid_x, mid_y, land_new_mid_x, land_new_mid_y, new_mid_x, new_mid_y, land_scale_ratio, land_anchor_offset_x, land_anchor_offset_y, scale_Type, scale_ratio, anchor_offset_x, anchor_offset_y, asize_offset_x, asize_offset_y, mfaceCount, imageName);
        } while (false);
        return i_err;
    }

	virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
	{
		int i_err = HJ_OK;
		do
		{
			HJ_JSON_BASE_SERIAL(m_obj, i_obj, mframeCount, radius_Type, mid_Type, scale_Type, scale_ratio, anchor_offset_x, anchor_offset_y, asize_offset_x, asize_offset_y, mfaceCount, imageName);

            HJ_JSON_BASE_SERIAL_CONDITIONAL_NOTEQUAL(land_new_mid_x, std::numeric_limits<double>::min(), land_new_mid_x);
            HJ_JSON_BASE_SERIAL_CONDITIONAL_NOTEQUAL(land_new_mid_y, std::numeric_limits<double>::min(), land_new_mid_y);
            HJ_JSON_BASE_SERIAL_CONDITIONAL_NOTEQUAL(new_mid_x, std::numeric_limits<double>::min(), new_mid_x);
            HJ_JSON_BASE_SERIAL_CONDITIONAL_NOTEQUAL(new_mid_y, std::numeric_limits<double>::min(), new_mid_y);
            HJ_JSON_BASE_SERIAL_CONDITIONAL_NOTEQUAL(land_scale_ratio, std::numeric_limits<double>::min(), land_scale_ratio);
            HJ_JSON_BASE_SERIAL_CONDITIONAL_NOTEQUAL(land_anchor_offset_x, std::numeric_limits<int>::min(), land_anchor_offset_x);
            HJ_JSON_BASE_SERIAL_CONDITIONAL_NOTEQUAL(land_anchor_offset_y, std::numeric_limits<int>::min(), land_anchor_offset_y);

            HJ_JSON_BASE_SERIAL_CONDITIONAL_EQUAL(radius_Type, HJFaceuConstants::RotateType_Fixed, mradius);
            HJ_JSON_BASE_SERIAL_CONDITIONAL_EQUAL(mid_Type, HJFaceuConstants::AnchorAlignType_Fixed, mid_x);
            HJ_JSON_BASE_SERIAL_CONDITIONAL_EQUAL(mid_Type, HJFaceuConstants::AnchorAlignType_Fixed, mid_y);
		} while (false);
		return i_err;
	}

    //virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr);
    int update();
    int draw(std::vector<HJPointf> i_points, int width, int height);
    void setMaxCount()
    {
        m_bMaxCount = true; 
    }
    bool isMaxCount() const
    {
        return m_bMaxCount;
    }
    
    int onParseReady();
    int onDataReady(unsigned char* data, int width, int height, int nrComponents);

    int mframeCount = 0;
    int radius_Type = HJFaceuConstants::RotateType_Eyes;
    int mradius = 0;
    int mid_Type = HJFaceuConstants::AnchorAlignType_Eyebrows;

    int scale_Type = HJFaceuConstants::ScaleType_EyeDistance;
    double scale_ratio = std::numeric_limits<double>::min();
    int anchor_offset_x = 0;
    int anchor_offset_y = 0;
    int asize_offset_x = 0;
    int asize_offset_y = 0;
    int mfaceCount = 1;

    double mid_x = 0.0;
    double mid_y = 0.0;
    
    double land_new_mid_x = std::numeric_limits<double>::min();
    double land_new_mid_y = std::numeric_limits<double>::min();
    double new_mid_x = std::numeric_limits<double>::min();
    double new_mid_y = std::numeric_limits<double>::min();
    double land_scale_ratio = std::numeric_limits<double>::min();
    int land_anchor_offset_x = std::numeric_limits<int>::min();
    int land_anchor_offset_y = std::numeric_limits<int>::min();

    std::string imageName = "";
    bool m_bDisable = false;

    //add every image absolute path
    std::vector<std::string> m_imagePaths;
    int m_procIdx = 0;    
    bool m_bMaxCount = false;
    int m_curLoopIdx = 0;
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;

    HJFaceuStrategy getStrategy() const
    {
        return m_strategy;
    }
    void setStrategy(HJFaceuStrategy i_strategy)
    {
        m_strategy = i_strategy;
    }
private:
    int priDraw(float * i_mvp);
    void priOnDestroy();
    bool priIsDrawReady();
    int priUpdate(const HJRawImageDataInfo::Ptr& i_data);
    HJFaceuStrategy m_strategy = HJFaceuStrategy_ExclusiveTexture;
    bool m_bCreateTexture = false;
    GLuint m_textureId = 0;
    HJSPBuffer::Ptr m_rbgabuffer = nullptr; 
    HJAsyncCache<HJRawImageDataInfo::Ptr> m_cache;
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

	virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
	{
		int i_err = HJ_OK;
		do
		{
			HJ_JSON_BASE_SERIAL(m_obj, i_obj, Name, ID, Type, loop, music);

            HJ_JSON_ARRAY_SERIAL(m_obj, i_obj, texture);

            HJ_JSON_BASE_SERIAL_CONDITIONAL_NOTEQUAL(framerate, std::numeric_limits<int>::min(), framerate);

            if (!restart.empty())
            {
                HJ_JSON_ARRAY_PLAIN_SERIAL(m_obj, i_obj, restart);
            }
		} while (false);
		return i_err;
	}

    int parse(HJBaseParam::Ptr i_param);
    int update();
    int draw(const std::vector<HJPointf>& i_points, int width, int height);
    //virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr);

    std::string Name = "";
    std::string ID = "";        
    int Type = 0;
    int loop = 1;
    std::string music = ""; 
    int framerate = std::numeric_limits<int>::min();

    std::vector<int> restart;
    std::vector<HJFaceuTextureInfo::Ptr> texture;

    HJFaceuStrategy getStrategy() const
    {
        return m_strategy;
    }
    void setStrategy(HJFaceuStrategy i_strategy)
    {
        m_strategy = i_strategy;
    }
    void setInsName(const std::string& i_name)
    {
        m_insName = i_name;
    }
    std::string getInsName() const
    {
        return m_insName;
    }
private:
    void priStat(int64_t t0, int i_fps);
    void priOnParseEnd(const HJBaseParam::Ptr& i_param);
    void priDone();
    int priCreateImgPaths();
    bool m_bDone = false;
    std::string m_rootPath = "";
    std::shared_ptr<HJTimerThreadPool> m_threadTimer = nullptr;
    HJListener m_renderListener = nullptr;
    HJFaceuStrategy m_strategy = HJFaceuStrategy_ExclusiveTexture;
    bool m_bUseSharedCtxTexture = false;
    std::string m_insName = "";
    int64_t m_statIdx = 0;
    int64_t m_statFirstTime = -1;
    int64_t m_statElapseSum = 0;
};

NS_HJ_END



