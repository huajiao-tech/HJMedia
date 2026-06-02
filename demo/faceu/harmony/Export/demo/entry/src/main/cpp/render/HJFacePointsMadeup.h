#pragma once

#include <vector>
#include <memory>

template<typename T>
struct HJPoint
{
    T x;
    T y;
};
using HJPointf = HJPoint<float>;

class HJFacePointsReal
{
public:
    HJFacePointsReal() = default;
    virtual ~HJFacePointsReal() = default;

    HJFacePointsReal(int w, int h, bool i_bContainsFace)
    {
        m_width = w;
        m_height = h;
        m_bContainsFace = i_bContainsFace;
    }
    bool isContainFace()
    {
        return m_bContainsFace;
    }
    void add(HJPointf i_point)
    {
        m_filterPoints.push_back(i_point);
    }
    void copy(std::vector<HJPointf> i_points)
    {
        m_filterPoints = i_points;
    }
    const std::vector<HJPointf>& getFilterPt()
    {
        return m_filterPoints;
    }
    int width()
    {
        return m_width;
    }
    int height()
    {
        return m_height;
    }
private:
    int m_width = 0;
    int m_height = 0;
    bool m_bContainsFace = false;
    std::vector<HJPointf> m_filterPoints;
};

class HJFacePointsReal;

class HJFacePointsMadeup 
{
public:
    HJFacePointsMadeup();
    virtual ~HJFacePointsMadeup();

    std::shared_ptr<HJFacePointsReal> getFacePoints(int i_imgIdx, int &o_imgIdx);
    int getFrameCnt();
    int getWidth();
    int getHeight();
private:

    void priConstruct(HJPointf f0, HJPointf f1, HJPointf f2, HJPointf f3, HJPointf f4, HJPointf f5, HJPointf f6, HJPointf f7, HJPointf f8);

    std::vector<std::shared_ptr<HJFacePointsReal>> m_facePointsVector;

    int m_width = 0;
    int m_height = 0;
    int m_index = 0;
};





