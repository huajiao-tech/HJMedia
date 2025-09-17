#pragma once

#include "HJRteComSource.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJRteComSource::HJRteComSource()
{
}
HJRteComSource::~HJRteComSource()
{
}
bool HJRteComSource::isRenderReady()
{
	return false;
}
int HJRteComSource::update(HJBaseParam::Ptr i_param)
{
	return 0;
}
//int HJRteComSource::render(HJBaseParam::Ptr i_param)
//{
//	return 0;
//}

////////////////////////////////////////////////////////////////////
HJRteComSourceBridge2::HJRteComSourceBridge2()
{

}
HJRteComSourceBridge2::~HJRteComSourceBridge2()
{
}
bool HJRteComSourceBridge2::isRenderReady()
{
	return false;
}
int HJRteComSourceBridge2::update(HJBaseParam::Ptr i_param)
{
	return 0;
}
//int HJRteComSourceBridge2::render(HJBaseParam::Ptr i_param)
//{
//	return 0;
//}

////////////////////////////////////////////////////////////////////
HJRteComSourcePngSeq::HJRteComSourcePngSeq()
{

}
HJRteComSourcePngSeq::~HJRteComSourcePngSeq()
{

}

bool HJRteComSourcePngSeq::isRenderReady()
{
	return false;
}
int HJRteComSourcePngSeq::update(HJBaseParam::Ptr i_param)
{
	return 0;
}
//int HJRteComSourcePngSeq::render(HJBaseParam::Ptr i_param)
//{
//	return 0;
//}


NS_HJ_END



