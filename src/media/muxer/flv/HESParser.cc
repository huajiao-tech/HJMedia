//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJESParser.h"
#include "HJESHEVCParser.h"
#include "HJESAV1Parser.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJESParser::HJESParser()
{
}

HJESParser::~HJESParser()
{
}

bool HJESParser::has_start_code(const uint8_t* data)
{
	if (data[0] != 0 || data[1] != 0)
		return false;

	return data[2] == 1 || (data[2] == 0 && data[3] == 1);
}

/* NOTE: I noticed that FFmpeg does some unusual special handling of certain
 * scenarios that I was unaware of, so instead of just searching for {0, 0, 1}
 * we'll just use the code from FFmpeg - http://www.ffmpeg.org/ */
const uint8_t* HJESParser::avc_find_startcode_internal(const uint8_t* p, const uint8_t* end)
{
	const uint8_t* a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	for (end -= 3; p < end; p += 4) {
		uint32_t x = *(const uint32_t*)p;

		if ((x - 0x01010101) & (~x) & 0x80808080) {
			if (p[1] == 0) {
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}

			if (p[3] == 0) {
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (end += 3; p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end + 3;
}

const uint8_t* HJESParser::nal_find_startcode(const uint8_t* p, const uint8_t* end)
{
	const uint8_t* out = avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1])
		out--;
	return out;
}

int HJESParser::compute_avc_keyframe_priority(const uint8_t* nal_start,
	bool* is_keyframe, int priority)
{
	const int type = nal_start[0] & 0x1F;
	if (type == HJ_NAL_SLICE_IDR)
		*is_keyframe = true;

	const int new_priority = nal_start[0] >> 5;
	if (priority < new_priority)
		priority = new_priority;

	return priority;
}

HJBuffer::Ptr HJESParser::serialize_avc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority)
{
	HJBuffer::Ptr buffer = std::make_shared<HJBuffer>(size);
	const uint8_t* const end = data + size;
	const uint8_t* nal_start = nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		*priority = compute_avc_keyframe_priority(
			nal_start, is_keyframe, *priority);

		const uint8_t* const nal_end =
			nal_find_startcode(nal_start, end);
		const size_t nal_size = nal_end - nal_start;
		buffer->wb32((uint32_t)nal_size);
		buffer->write((uint8_t *)nal_start, nal_size);
		nal_start = nal_end;
	}
	return buffer;
}

HJBuffer::Ptr HJESParser::parse_avc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority)
{
	HJBuffer::Ptr buffer = std::make_shared<HJBuffer>(data, size);
	size_t head_size = 4;
	const uint8_t* nal_start = data;
	const uint8_t* nal_end = data + size;

	while (nal_start + head_size <= nal_end) {
		size_t nal_size = AV_RB32(nal_start);
		nal_start += head_size;

		*priority = compute_avc_keyframe_priority(
			nal_start, is_keyframe, *priority);

		nal_start += nal_size;
	}

	return buffer;
}

HJBuffer::Ptr HJESParser::proc_avc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority)
{
	if (size <= 6) {
		return nullptr;
	}
	HJBuffer::Ptr buffer = nullptr;
	if (has_start_code(data)) {
		buffer = HJESParser::serialize_avc_data(data, size, is_keyframe, priority);
	} else { 
		buffer = HJESParser::parse_avc_data(data, size, is_keyframe, priority);
	}
	return buffer;
}

int HJESParser::compute_hevc_keyframe_priority(const uint8_t* nal_start, bool* is_keyframe, int priority)
{
	// HEVC contains NAL unit specifier at [6..1] bits of
// the byte next to the startcode 0x000001
	const int type = (nal_start[0] & 0x7F) >> 1;

	// Mark IDR slices as key-frames and set them to highest
	// priority if needed. Assume other slices are non-key
	// frames and set their priority as high
	if (type >= HJ_HEVC_NAL_BLA_W_LP &&
		type <= HJ_HEVC_NAL_RSV_IRAP_VCL23) {
		*is_keyframe = 1;
		priority = HJ_NAL_PRIORITY_HIGHEST;
	}
	else if (type >= HJ_HEVC_NAL_TRAIL_N &&
		type <= HJ_HEVC_NAL_RASL_R) {
		if (priority < HJ_NAL_PRIORITY_HIGH)
			priority = HJ_NAL_PRIORITY_HIGH;
	}

	return priority;
}

HJBuffer::Ptr HJESParser::serialize_hevc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority)
{
	HJBuffer::Ptr buffer = std::make_shared<HJBuffer>(size);
	const uint8_t* const end = data + size;
	const uint8_t* nal_start = nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		*priority = compute_hevc_keyframe_priority(
			nal_start, is_keyframe, *priority);

		const uint8_t* const nal_end =
			nal_find_startcode(nal_start, end);
		const size_t nal_size = nal_end - nal_start;
		buffer->wb32((uint32_t)nal_size);
		buffer->write((uint8_t *)nal_start, nal_size);
		nal_start = nal_end;
	}
	return buffer;
}

HJBuffer::Ptr HJESParser::parse_hevc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority)
{
	HJBuffer::Ptr buffer = std::make_shared<HJBuffer>(data, size);
	size_t head_size = 4;
	const uint8_t* nal_start = data;
	const uint8_t* nal_end = data + size;

	while (nal_start + head_size <= nal_end) {
		size_t nal_size = AV_RB32(nal_start);
		nal_start += head_size;

		*priority = compute_hevc_keyframe_priority(
			nal_start, is_keyframe, *priority);

		nal_start += nal_size;
	}

	return buffer;
}

HJBuffer::Ptr HJESParser::proc_hevc_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority)
{
	if (size <= 6) {
		return nullptr;
	}
	HJBuffer::Ptr buffer = nullptr;
	if (has_start_code(data)) {
		buffer = HJESParser::serialize_hevc_data(data, size, is_keyframe, priority);
	}
	else {
		buffer = HJESParser::parse_hevc_data(data, size, is_keyframe, priority);
	}
	return buffer;
}

HJBuffer::Ptr HJESParser::serialize_av1_data(const uint8_t* data, size_t size, bool* is_keyframe, int* priority)
{
	return HJESAV1Parser::serializeData(data, size, is_keyframe, priority);
}

void HJESParser::get_sps_pps(const uint8_t* data, size_t size, const uint8_t** sps, size_t* sps_size, const uint8_t** pps, size_t* pps_size)
{
	const uint8_t* nal_start, * nal_end;
	const uint8_t* end = data + size;
	int type;

	nal_start = nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		nal_end = nal_find_startcode(nal_start, end);

		type = nal_start[0] & 0x1F;
		if (type == HJ_NAL_SPS) {
			*sps = nal_start;
			*sps_size = nal_end - nal_start;
		}
		else if (type == HJ_NAL_PPS) {
			*pps = nal_start;
			*pps_size = nal_end - nal_start;
		}

		nal_start = nal_end;
	}
}

HJBuffer::Ptr HJESParser::parse_avc_header(const uint8_t* data, size_t size)
{
	if (size <= 6) {
		return nullptr;
	}
	const uint8_t* sps = NULL, * pps = NULL;
	size_t sps_size = 0, pps_size = 0;

	if (!has_start_code(data)) {
		return std::make_shared<HJBuffer>(data, size);
	}
	get_sps_pps(data, size, &sps, &sps_size, &pps, &pps_size);
	if (!sps || !pps || sps_size < 4) {
		return nullptr;
	}
	HJBuffer::Ptr buffer = std::make_shared<HJBuffer>(size + 32);
	buffer->w8(0x01);
	buffer->write((uint8_t *)(sps + 1), 3);
	buffer->w8(0xff);
	buffer->w8(0xe1);

	buffer->wb16((uint16_t)sps_size);
	buffer->write((uint8_t*)sps, sps_size);
	buffer->w8(0x01);
	buffer->wb16((uint16_t)pps_size);
	buffer->write((uint8_t*)pps, pps_size);

	return buffer;
}

HJBuffer::Ptr HJESParser::parse_hevc_header(const uint8_t* data, size_t size)
{
	return HJESHEVCParser::parseHeader(data, size);
}

HJBuffer::Ptr HJESParser::parse_av1_header(const uint8_t* data, size_t size)
{
	return HJESAV1Parser::parseHeader(data, size);
}

NS_HJ_END