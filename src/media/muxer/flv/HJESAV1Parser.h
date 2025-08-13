//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJBuffer.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJESAV1Parser
{
public:
	static HJBuffer::Ptr parseHeader(const uint8_t* data, size_t size);
	static HJBuffer::Ptr serializeData(const uint8_t* data, size_t size, bool* is_keyframe, int* priority);
};

NS_HJ_END