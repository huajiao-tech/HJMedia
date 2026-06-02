#pragma once

#include <vector>
#include "HJMediaUtils.h"

NS_HJ_BEGIN

typedef struct HJSRRet
{
	int64_t m_systemTime = 0;
	int64_t m_elapseMs = 0;
	int64_t m_timeStamp = 0;
} HJSRRet;

NS_HJ_END