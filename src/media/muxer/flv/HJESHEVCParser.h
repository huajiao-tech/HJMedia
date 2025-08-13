//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJBuffer.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJESHEVCParser
{
public:
	static HJBuffer::Ptr parseHeader(const uint8_t* data, size_t size);
};

NS_HJ_END