#pragma once

#include "HJPrerequisites.h"
#include "HJMediaUtils.h"
#include "HJJsonBase.h"
#include "HJReflCppJson.h"

NS_HJ_BEGIN

class HJFacePointsReal : public HJJsonBase
{
public:
	HJ_DEFINE_CREATE(HJFacePointsReal);
    HJFacePointsReal() = default;
	virtual ~HJFacePointsReal() = default;  

    virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, m_width, m_height, m_bContainsFace, m_timestamp, m_bDebugPoints, m_systemTime, m_elapsedTime);
            i_err = m_obj->forEach("m_filterPoints", [&](const HJYJsonObject::Ptr& subObj)
            { 
                int ret = HJ_OK; 
                double x = 0.0;
                double y = 0.0;
                ret = subObj->getMember("x", x);
                if (ret < 0)
                {
                    return ret;
                }
                ret = subObj->getMember("y", y);
                if (ret < 0)
                {
                    return ret;
                }
                m_filterPoints.emplace_back(HJPointf{(float)x, (float)y});
                return ret; 
            }); 
            if (i_err == HJErrNotExist)
            {
                i_err = HJ_OK; 
            }
        } while (false);
        return i_err;
    }
    virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
			HJ_JSON_BASE_SERIAL(m_obj, i_obj, m_width, m_height, m_bContainsFace, m_timestamp, m_bDebugPoints, m_systemTime, m_elapsedTime);

			std::vector<HJYJsonObject::Ptr> jItems;
			for (auto& info : m_filterPoints)
			{
				auto itemObj = std::make_shared<HJYJsonObject>("m_filterPoints" + std::string("_item"), m_obj);
				i_err = itemObj->setMember("x", (double)info.x);
				if (i_err < 0)
				{
					return i_err;
				}
				i_err = itemObj->setMember("y", (double)info.y);
				if (i_err < 0)
				{
					return i_err;
				}
				jItems.push_back(itemObj);
			}
			m_obj->setMember("m_filterPoints", jItems);
        } while (false);
        return i_err;
    }
    HJFacePointsReal(int w, int h, bool i_bContainsFace, int64_t i_timestamp = 0):
        m_width(w),
        m_height(h),
        m_bContainsFace(i_bContainsFace),
        m_timestamp(i_timestamp)
    {
    }
    bool isContainFace() const
    {
        return m_bContainsFace;
    }
    void add(HJPointf i_point)
    {
        m_filterPoints.push_back(std::move(i_point));
    }
    void copy(std::vector<HJPointf> i_points)
    {
        m_filterPoints = i_points;
    }
    const std::vector<HJPointf> &getFilterPt()
    {
        return m_filterPoints;
    }
    int width() const 
    {
        return m_width;
    }
    int height() const 
    {
        return m_height;
    }
    void setWidth(int i_width)
    {
        m_width = i_width;
    }
    void setHeight(int i_height)
    {
        m_height = i_height;
    }
    void setTimestamp(int64_t i_timestamp)
    {
        m_timestamp = i_timestamp;
    }
    int64_t getTimestamp() const
    {
        return m_timestamp;
    }
    void setIsDebugPoints(bool i_bDebugPoints)
    {
        m_bDebugPoints = i_bDebugPoints;
    }
    bool isDebugPoints() const
    {
        return m_bDebugPoints;
    }
    void setSystemTime(int64_t i_systemTime)
    {
        m_systemTime = i_systemTime;
    }
    int64_t getSystemTime() const
    {
        return m_systemTime;
    }
    void setElapsedTime(int64_t i_elapsedTime)
    {
        m_elapsedTime = i_elapsedTime;
    }
    int64_t getElapsedTime() const
    {
        return m_elapsedTime;
    }
private:
    int m_width = 0;
    int m_height = 0;
    bool m_bContainsFace = false;
    std::vector<HJPointf> m_filterPoints;
    int64_t m_timestamp = 0;
    bool m_bDebugPoints = false;
    int64_t m_systemTime = 0;
    int64_t m_elapsedTime = 0;
};


//class HJSingleFaceRect : public HJReflCppJsonInterpreter<HJSingleFaceRect>
//{
//public:
//    HJ_DEFINE_CREATE(HJSingleFaceRect);
//    HJSingleFaceRect() = default;
//    virtual ~HJSingleFaceRect() = default;
//
//    double x = 0.f;
//    double y = 0.f;
//    double width = 0.f;
//    double height = 0.f;
//};

class HJSingleFacePointsReal : public HJReflCppJsonInterpreter<HJSingleFacePointsReal>
{
public:
    HJ_DEFINE_CREATE(HJSingleFacePointsReal);
    HJSingleFacePointsReal() = default;
    virtual ~HJSingleFacePointsReal() = default;

    void add(HJPointf i_point)
    {
        m_filterPoints.push_back(std::move(i_point));
    }
    void copy(std::vector<HJPointf> i_points)
    {
        m_filterPoints = i_points;
    }
    const std::vector<HJPointf>& getFilterPt()
    {
        return m_filterPoints;
    }
    const HJRectf& getFaceRect()
    {
        return m_faceRect;
    }
    void setFaceRect(HJRectf i_faceRect)
    {
        m_faceRect = i_faceRect;
    }
    void setFaceRect(float i_x, float i_y, float i_width, float i_height)
    {
        m_faceRect.x = i_x;
        m_faceRect.y = i_y;
        m_faceRect.w = i_width;
        m_faceRect.h = i_height;
    }

    std::vector<HJPointf> m_filterPoints;
    HJRectf m_faceRect;
};

class HJMoreFacePointsReal : public HJReflCppJsonInterpreter<HJMoreFacePointsReal>
{
public:

    HJ_DEFINE_CREATE(HJMoreFacePointsReal);
    HJMoreFacePointsReal() = default;
    virtual ~HJMoreFacePointsReal() = default;

    HJMoreFacePointsReal(int w, int h, bool i_bContainsFace, int64_t i_timestamp = 0) :
        m_width(w),
        m_height(h),
        m_bContainsFace(i_bContainsFace),
        m_timestamp(i_timestamp)
    {
    }

    static std::shared_ptr<HJMoreFacePointsReal> cvtFrom(std::shared_ptr<HJFacePointsReal> i_pointreal);
    static std::shared_ptr<HJMoreFacePointsReal> getFakePoints(bool i_bDebug = false);

    bool isContainFace() const
    {
        return m_bContainsFace;
    }
    void add(HJSingleFacePointsReal::Ptr i_point)
    {
        m_points.push_back(std::move(i_point));
    }
    std::vector<HJSingleFacePointsReal::Ptr>& getPoints()
    {
        return m_points;
    }
    int width() const
    {
        return m_width;
    }
    int height() const
    {
        return m_height;
    }
    void setWidth(int i_width)
    {
        m_width = i_width;
    }
    void setHeight(int i_height)
    {
        m_height = i_height;
    }
    void setTimestamp(int64_t i_timestamp)
    {
        m_timestamp = i_timestamp;
    }
    int64_t getTimestamp() const
    {
        return m_timestamp;
    }
    void setIsDebugPoints(bool i_bDebugPoints)
    {
        m_bDebugPoints = i_bDebugPoints;
    }
    bool isDebugPoints() const
    {
        return m_bDebugPoints;
    }
    void setSystemTime(int64_t i_systemTime)
    {
        m_systemTime = i_systemTime;
    }
    int64_t getSystemTime() const
    {
        return m_systemTime;
    }
    void setElapsedTime(int64_t i_elapsedTime)
    {
        m_elapsedTime = i_elapsedTime;
    }
    int64_t getElapsedTime() const
    {
        return m_elapsedTime;
    }

    int m_width = 0;
    int m_height = 0;
    bool m_bContainsFace = false;

    int64_t m_timestamp = 0;
    bool m_bDebugPoints = false;
    int64_t m_systemTime = 0;
    int64_t m_elapsedTime = 0;

    std::vector<HJSingleFacePointsReal::Ptr> m_points;
};

NS_HJ_END

REFL_AUTO_SIMPLE(HJ::HJSingleFacePointsReal, m_filterPoints, m_faceRect)
REFL_AUTO_SIMPLE(HJ::HJMoreFacePointsReal, m_width, m_height, m_bContainsFace, m_timestamp, m_bDebugPoints, m_systemTime, m_elapsedTime, m_points)
