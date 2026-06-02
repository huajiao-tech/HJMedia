#pragma once

#include "HJReflCppJson.h"
#include "HJComUtils.h"

#if 0

NS_HJ_BEGIN
class HJRteGraphConfigFaceuInfo : public HJReflCppJsonInterpreter<HJRteGraphConfigFaceuInfo>
{
public:
	std::string faceuUrl = "";
	bool bDebugPoints = false;
	bool bUseFakePoints = false;
};
NS_HJ_END

REFL_AUTO_SIMPLE(HJ::HJRteGraphConfigFaceuInfo, faceuUrl, bDebugPoints, bUseFakePoints)

#endif


