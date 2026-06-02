#pragma once

#include "HJPrerequisites.h"
#include "HJMediaUtils.h"

NS_HJ_BEGIN

class HJFacePointsReal;

class HJFacePointsMadeup 
{
public:
    HJ_DEFINE_CREATE(HJFacePointsMadeup);
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



NS_HJ_END



