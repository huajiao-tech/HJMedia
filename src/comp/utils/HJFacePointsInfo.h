#pragma once

#include "HJPrerequisites.h"
#include "HJReflCppJson.h"

NS_HJ_BEGIN

class HJFacePointRectInfo 
{
public:
	HJ_DEFINE_CREATE(HJFacePointRectInfo);
	int left = 0;
	int top = 0;
	int width = 0;
	int height = 0;
};
class HJFacePointPoseInfo
{
public:
	HJ_DEFINE_CREATE(HJFacePointPoseInfo);
	double yaw = 0.0;
	double pitch = 0.0;
	double roll = 0.0;
};

class HJFaceSinglePointItem
{
public:
	HJ_DEFINE_CREATE(HJFaceSinglePointItem);
	int x = 0;
	int y = 0;
};

class HJFacePointItem 
{
public:
	HJ_DEFINE_CREATE(HJFacePointItem);
	double probability = 0.0;
	int block = -1;
	HJFacePointRectInfo rect;
	HJFacePointPoseInfo pose;
	std::vector<HJFaceSinglePointItem::Ptr> points;
};
class HJFacePointsInfo : public HJReflCppJsonInterpreter<HJFacePointsInfo>
{
public:
	HJ_DEFINE_CREATE(HJFacePointsInfo);
	std::vector<HJFacePointItem::Ptr> pointItems;
};

NS_HJ_END

REFL_AUTO_SIMPLE(HJ::HJFacePointRectInfo, left, top, width, height)
REFL_AUTO_SIMPLE(HJ::HJFacePointPoseInfo, yaw, pitch, roll)
REFL_AUTO_SIMPLE(HJ::HJFaceSinglePointItem, x, y)
REFL_AUTO_SIMPLE(HJ::HJFacePointItem, probability, block, rect, pose, points)
REFL_AUTO_SIMPLE(HJ::HJFacePointsInfo, pointItems)