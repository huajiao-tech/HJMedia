//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaInfo.h"
#include "HJMediaFrame.h"

NS_HJ_BEGIN
//***********************************************************************************//
enum {
	HJ_NAL_PRIORITY_DISPOSABLE = 0,
	HJ_NAL_PRIORITY_LOW = 1,
	HJ_NAL_PRIORITY_HIGH = 2,
	HJ_NAL_PRIORITY_HIGHEST = 3,
};

class HJESParser : public HJObject
{
public:
	HJESParser();
	~HJESParser();

	static const uint8_t* nal_find_startcode(const uint8_t* p, const uint8_t* end);

	static HJBuffer::Ptr serialize_avc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority);
	static HJBuffer::Ptr serialize_hevc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority);
	static HJBuffer::Ptr serialize_av1_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority);

	static HJBuffer::Ptr parse_avc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority);
	static HJBuffer::Ptr parse_hevc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority);

	static HJBuffer::Ptr proc_avc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority);
	static HJBuffer::Ptr proc_hevc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority);
	//
	static HJBuffer::Ptr parse_avc_header(const uint8_t* data, size_t size);
	static HJBuffer::Ptr parse_hevc_header(const uint8_t* data, size_t size);
	static HJBuffer::Ptr parse_av1_header(const uint8_t* data, size_t size);
protected:
	static bool has_start_code(const uint8_t* data);
	static const uint8_t* avc_find_startcode_internal(const uint8_t* p, const uint8_t* end);
	static int compute_avc_keyframe_priority(const uint8_t* nal_start, bool* is_keyframe, int priority);
	static int compute_hevc_keyframe_priority(const uint8_t* nal_start, bool* is_keyframe, int priority);
	static void get_sps_pps(const uint8_t* data, size_t size, const uint8_t** sps, size_t* sps_size, const uint8_t** pps, size_t* pps_size);
};

NS_HJ_END