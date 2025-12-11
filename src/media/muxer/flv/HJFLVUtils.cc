//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFLVUtils.h"
#include "HJFFUtils.h"
#include "HJFLog.h"
#include "HJESParser.h"
#include "HJSEIWrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "librtmp/rtmp.h"
#ifdef __cplusplus
};
#endif

#define	HJ_FLV_META_DATA_SIZE_DEFAULT	512
#define	HJ_FLV_HEADER_SIZE_DEFAULT		1024
#define	HJ_FLV_TAG_SIZE_DEFAULT		32

#define HJ_VIDEODATA_AVCVIDEOPACKET	7.0
#define HJ_VIDEODATA_AV1VIDEOPACKET	1635135537.0
#define HJ_VIDEODATA_HEVCVIDEOPACKET	1752589105.0
#define HJ_AUDIODATA_AAC				10.0

NS_HJ_BEGIN
//***********************************************************************************//
int HJFLVPacket::init(HJMediaFrame::Ptr frame, int64_t tsOffset)  
{
	int res = HJ_OK;
	AVPacket* pkt = (AVPacket*)frame->getAVPacket();
	if (!pkt) {
		return HJErrInvalidParams;
	}
	m_pts = frame->m_pts - tsOffset;
	m_dts = frame->m_dts - tsOffset;
	m_dtsUsec = frame->getDTSUS() - HJ_MS_TO_US(tsOffset);
	m_dtsSysUsec = HJCurrentSteadyMS();
	m_extraTS = frame->getExtraTS();
	m_keyFrame = frame->isKeyFrame();
	m_trackIdx = 0; //frame->m_streamIndex;
	//
	if (frame->isVideo()) 
	{
		m_type = HJMEDIA_TYPE_VIDEO;
		m_codecid = frame->getVideoInfo()->getCodecID();
		//
		uint8_t* data = pkt->data;
		int data_size = pkt->size;
		//SEI
		std::vector<uint8_t> nalsWithSeiData{};
		HJSEINals::Ptr seiNals{};
		if (frame->haveValue(HJMediaFrame::STORE_KEY_SEIINFO)) 
		{
			seiNals = frame->getValue<HJSEINals::Ptr>(HJMediaFrame::STORE_KEY_SEIINFO);
			if (seiNals && !seiNals->empty()) 
			{
				bool isH265 = (AV_CODEC_ID_H265 == m_codecid);
				nalsWithSeiData = HJSEIWrapper::insertSEIToFrame(pkt->data, pkt->size, seiNals->getDatas(), isH265);
				if (nalsWithSeiData.size() <= 0) {
					HJLoge("error, insert sei data failed");
					return HJErrFatal;
				}
				data = nalsWithSeiData.data();
				data_size = nalsWithSeiData.size();
			}
		}
		//
		switch (m_codecid)
		{
		case AV_CODEC_ID_H264: {
			m_data = HJESParser::proc_avc_data(data, data_size, &m_keyFrame, &m_priority);
			break;
		}
		case AV_CODEC_ID_H265: {
			m_data = HJESParser::proc_hevc_data(data, data_size, &m_keyFrame, &m_priority);
			break;
		}
		case AV_CODEC_ID_AV1: {
			m_data = HJESParser::serialize_av1_data(pkt->data, pkt->size, &m_keyFrame, &m_priority);
			break;
		}
		default:
			HJLoge("not support");
			res = HJErrNotSupport;
			break;
		}
	} else { //audio
		m_type = HJMEDIA_TYPE_AUDIO;
		m_codecid = frame->getAudioInfo()->getCodecID();
		m_priority = HJFLV_PKT_PRIORITY_HIGH;
		
		m_data = std::make_shared<HJBuffer>(pkt->data, pkt->size);
	}

	return res;
}

HJFLVPacket::Ptr HJFLVPacket::makeAudioPacket()
{
	HJFLVPacket::Ptr packet = std::make_shared<HJFLVPacket>();
	packet->m_type = HJMEDIA_TYPE_AUDIO;
	packet->m_dtsSysUsec = HJCurrentSteadyMS();
	return packet;
}

HJFLVPacket::Ptr HJFLVPacket::makeVideoPacket(int codecid, bool isKeyFrame)
{
	HJFLVPacket::Ptr packet = std::make_shared<HJFLVPacket>();
	packet->m_type = HJMEDIA_TYPE_VIDEO;
	packet->m_codecid = codecid;
	packet->m_keyFrame = isKeyFrame;
	packet->m_dtsSysUsec = HJCurrentSteadyMS();
	return packet;
}

std::string HJFLVPacket::formatInfo()
{
	std::string info = HJFMT("type:{}, codec:{}, pts:{}, dts:{}, key frame:{}, priority:{}, track idx:{}, size:{}", 
		HJMediaType2String(m_type), AVCodecIDToString((AVCodecID)m_codecid), m_pts, m_dts, m_keyFrame, m_priority, m_trackIdx, m_data ? m_data->size() : 0);
	return info;
}

//***********************************************************************************//
int HJFLVPacketQueue::push(HJFLVPacket::Ptr packet)
{
	HJ_AUTO_LOCK(m_mutex);
	m_packets.emplace_back(std::move(packet));
	return HJ_OK;
}

HJFLVPacket::Ptr HJFLVPacketQueue::pop()
{
	HJ_AUTO_LOCK(m_mutex);
	HJFLVPacket::Ptr packet = m_packets.front();
	m_packets.pop_front();
	return packet;
}

HJFLVPacket::Ptr HJFLVPacketQueue::front()
{
	HJ_AUTO_LOCK(m_mutex);
	return m_packets.front();
}

HJFLVPacket::Ptr HJFLVPacketQueue::findFirstVideoPacket()
{
	HJ_AUTO_LOCK(m_mutex);
	HJFLVPacket::Ptr first = nullptr;
	for (auto packet : m_packets) {
		if (HJMEDIA_TYPE_VIDEO == packet->m_type) {
			first = packet;
		}
	}
	return first;
}

//size_t HJFLVPacketQueue::drop(int priority)
//{
//	HJ_AUTO_LOCK(m_mutex);
//	size_t dropNum = 0;
//	for (auto it = m_packets.begin(); it != m_packets.end();) {
//		HJFLVPacket::Ptr packet = *it;
//		if (HJMEDIA_TYPE_VIDEO == packet->m_type && packet->m_dropPriority < priority) {
//			it = m_packets.erase(it);
//			dropNum++;
//		} else {
//			it++;
//		}
//	}
//	return dropNum;
//}

size_t HJFLVPacketQueue::size()
{
	HJ_AUTO_LOCK(m_mutex);
	return m_packets.size();
}


//***********************************************************************************//
#define HJ_VIDEO_FRAMETYPE_OFFSET 4
enum HJFLVVideoFrametype {
	HJFLV_FT_KEY = 1 << HJ_VIDEO_FRAMETYPE_OFFSET,
	HJFLV_FT_INTER = 2 << HJ_VIDEO_FRAMETYPE_OFFSET,
};

// Y2023 spec
const uint8_t HJ_FRAME_HEADER_EX = 8 << HJ_VIDEO_FRAMETYPE_OFFSET;
enum HJFLVPacketType {
	HJFLV_PACKETTYPE_SEQ_START = 0,
	HJFLV_PACKETTYPE_FRAMES = 1,
	HJFLV_PACKETTYPE_SEQ_END = 2,
	HJFLV_PACKETTYPE_FRAMESX = 3,
	HJFLV_PACKETTYPE_METADATA = 4
};

enum HJFLVDataType {
	HJFLV_DATA_TYPE_NUMBER = 0,
	HJFLV_DATA_TYPE_STRING = 2,
	HJFLV_DATA_TYPE_OBJECT = 3,
	HJFLV_DATA_TYPE_OBJECT_END = 9,
};
//***********************************************************************************//
HJBuffer::Ptr HJFLVUtils::buildMetaDataTag(HJMediaInfo::Ptr mediaInfo, bool writeHeader)
{
	HJBuffer::Ptr metaData = buildMetaData(mediaInfo);
	if (!metaData) {
		return NULL;
	}
	HJBuffer::Ptr tag = std::make_shared<HJBuffer>(HJ_FLV_HEADER_SIZE_DEFAULT);
	if (writeHeader) {
		tag->write((uint8_t *)"FLV", 3);
		tag->w8(1);
		tag->w8(5);
		tag->wb32(9);
		tag->wb32(0);
	}
	uint32_t start_pos = (uint32_t)tag->size();
	tag->w8(RTMP_PACKET_TYPE_INFO);

	tag->wb24((uint32_t)metaData->size());
	tag->wb32(0);
	tag->wb24(0);

	tag->write(metaData->data(), metaData->size());

	tag->wb32((uint32_t)tag->size() - start_pos);

	return tag;
}

HJBuffer::Ptr HJFLVUtils::buildMetaData(HJMediaInfo::Ptr mediaInfo)
{
	HJVideoInfo::Ptr videoInfo = mediaInfo->getVideoInfo();
	HJAudioInfo::Ptr audioInfo = mediaInfo->getAudioInfo();
	HJBuffer::Ptr metaData = std::make_shared<HJBuffer>(HJ_FLV_META_DATA_SIZE_DEFAULT);
	char* enc = (char*)metaData->data();
	char* end = enc + metaData->capacity();

	enc_str(&enc, end, "@setDataFrame");
	enc_str(&enc, end, "onMetaData");

	*enc++ = AMF_ECMA_ARRAY;
	enc = AMF_EncodeInt32(enc, end, 20);

	enc_num_val(&enc, end, "duration", 0.0);
	enc_num_val(&enc, end, "fileSize", 0.0);

	if (videoInfo) {
		HJFLogi("video info: width:{}, height:{}, framerate:{}, bitrate:{}", videoInfo->m_width, videoInfo->m_height, videoInfo->m_frameRate, videoInfo->m_bitrate);

		enc_num_val(&enc, end, "width", (double)videoInfo->m_width);
		enc_num_val(&enc, end, "height", (double)videoInfo->m_height);

		double codecID = HJ_VIDEODATA_AVCVIDEOPACKET;
		if (videoInfo->m_codecID == AV_CODEC_ID_H264) {
			codecID = HJ_VIDEODATA_AVCVIDEOPACKET;
		} else if (videoInfo->m_codecID == AV_CODEC_ID_AV1) {
			codecID = HJ_VIDEODATA_AV1VIDEOPACKET;
		} else if (videoInfo->m_codecID == AV_CODEC_ID_H265) {
			codecID = HJ_VIDEODATA_HEVCVIDEOPACKET;
		}
		enc_num_val(&enc, end, "videocodecid", codecID);
		enc_num_val(&enc, end, "videodatarate", (double)videoInfo->m_bitrate);
		enc_num_val(&enc, end, "framerate", (double)videoInfo->m_frameRate);
	}
	if (audioInfo) 
	{
		HJFLogi("audio info: code id:{}, channels:{}, samplerate:{}, bytes per sample:{}", audioInfo->m_codecID, audioInfo->m_channels, audioInfo->m_samplesRate, audioInfo->m_bytesPerSample);

		if (AV_CODEC_ID_AAC != audioInfo->m_codecID) {
			HJLoge(HJFMT("not support audio codec id: {}", audioInfo->m_codecID));
			return nullptr;
		}
		enc_num_val(&enc, end, "audiocodecid", HJ_AUDIODATA_AAC);
		enc_num_val(&enc, end, "audiodatarate", (double)audioInfo->m_bitrate);
		enc_num_val(&enc, end, "audiosamplerate", (double)audioInfo->m_samplesRate);
		enc_num_val(&enc, end, "audiosamplesize", (double)(audioInfo->m_bytesPerSample * 8.0 / audioInfo->m_channels));	//16.0);
		enc_num_val(&enc, end, "audiochannels", (double)audioInfo->m_channels);

		enc_bool_val(&enc, end, "stereo", audioInfo->m_channels == 2);
		enc_bool_val(&enc, end, "2.1", audioInfo->m_channels == 3);
		enc_bool_val(&enc, end, "3.1", audioInfo->m_channels == 4);
		enc_bool_val(&enc, end, "4.0", audioInfo->m_channels == 4);
		enc_bool_val(&enc, end, "4.1", audioInfo->m_channels == 5);
		enc_bool_val(&enc, end, "5.1", audioInfo->m_channels == 6);
		enc_bool_val(&enc, end, "7.1", audioInfo->m_channels == 8);
	}
	enc_str_val(&enc, end, "encoder", HJ_VERSION);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;
	//
	size_t size = enc - (char *)metaData->data();
	metaData->setSize(size);

	return metaData;
}

HJBuffer::Ptr HJFLVUtils::buildAdditionalMetaTag()
{
	HJBuffer::Ptr metaData = buildAdditionalMetaData();
	if (!metaData) {
		return NULL;
	}
	HJBuffer::Ptr tag = std::make_shared<HJBuffer>(HJ_FLV_HEADER_SIZE_DEFAULT);

	tag->w8(RTMP_PACKET_TYPE_INFO); //18

	tag->wb24((uint32_t)metaData->size());
	tag->wb32(0);
	tag->wb24(0);

	tag->write(metaData->data(), metaData->size());

	tag->wb32((uint32_t)tag->size());

	return tag;
}

HJBuffer::Ptr HJFLVUtils::buildAdditionalMetaData()
{
	HJBuffer::Ptr meta = std::make_shared<HJBuffer>(HJ_FLV_META_DATA_SIZE_DEFAULT);

	meta->w8(AMF_STRING);
	meta->wstring("@setDataFrame");

	meta->w8(AMF_STRING);
	meta->wstring("onExpectAdditionalMedia");

	meta->w8(AMF_OBJECT);
	{
		meta->wstring("processingIntents");

		meta->w8(AMF_STRICT_ARRAY);
		meta->wb32(1);
		{
			meta->w8(AMF_STRING);
			meta->wstring("ArchiveProgramNarrationAudio");
		}

		/* ---- */
		meta->wstring("additionalMedia");

		meta->w8(AMF_OBJECT);
		{
			meta->wstring("stream0");

			meta->w8(AMF_OBJECT);
			{
				meta->wstring("type");

				meta->w8(AMF_NUMBER);
				meta->wbd(RTMP_PACKET_TYPE_AUDIO);

				/* ---- */
				meta->wstring("mediaLabels");

				meta->w8(AMF_OBJECT);
				{
					meta->wstring("contentType");

					meta->w8(AMF_STRING);
					meta->wstring("PNAR");
				}
				meta->wb24(AMF_OBJECT_END);
			}
			meta->wb24(AMF_OBJECT_END);
		}
		meta->wb24(AMF_OBJECT_END);

		/* ---- */
		meta->wstring("defaultMedia");

		meta->w8(AMF_OBJECT);
		{
			meta->wstring("audio");

			meta->w8(AMF_OBJECT);
			{
				meta->wstring("mediaLabels");

				meta->w8(AMF_OBJECT);
				{
					meta->wstring("contentType");

					meta->w8(AMF_STRING);
					meta->wstring("PRM");
				}
				meta->wb24(AMF_OBJECT_END);
			}
			meta->wb24(AMF_OBJECT_END);
		}
		meta->wb24(AMF_OBJECT_END);
	}
	meta->wb24(AMF_OBJECT_END);

	return meta;
}

HJBuffer::Ptr HJFLVUtils::buildAudioTag(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader)
{
	if (!packet) {
		HJLoge("error, packet have no audio data");
		return nullptr;
	}
	HJBuffer::Ptr es = packet->m_data;
	//if (!es->data() || !es->size()) {
	//	return nullptr;
	//}
	uint8_t* esDataPtr = NULL;
	size_t esDataSize = 0;
	if(es) {
        esDataPtr = es->data();
        esDataSize = es->size();
		auto adtsLength = HJFLVUtils::getADTSHeaderLength(esDataPtr, esDataSize);
		//
		esDataPtr += adtsLength;
		esDataSize -= adtsLength;
	}
	HJBuffer::Ptr tag = std::make_shared<HJBuffer>(esDataSize + HJ_FLV_TAG_SIZE_DEFAULT);
	int32_t time_ms = packet->m_dts - dts0ffset;

	//HJFLogi("build tag audio - time_ms:{}, dts0ffset:{}", time_ms, dts0ffset);
	tag->w8(RTMP_PACKET_TYPE_AUDIO);

	tag->wb24((uint32_t)esDataSize + 2);
	tag->wb24((uint32_t)time_ms);
	tag->w8((time_ms >> 24) & 0x7F);
	tag->wb24(0);

	/* these are the two extra bytes mentioned above */
	tag->w8(0xaf);
	tag->w8(isHeader ? 0 : 1);
	if (esDataPtr) {
		tag->write(esDataPtr, esDataSize);
	} 

	/* write tag size (starting byte doesn't count) */
	tag->wb32((uint32_t)tag->size());

	tag->setTimestamp(time_ms);
	tag->setName(AVCodecIDToString((AVCodecID)packet->m_codecid));

	return tag;
}

void HJFLVUtils::wu29(HJBuffer::Ptr buffer, uint32_t val)
{
	if (val <= 0x7F) {
		buffer->w8(val);
	}
	else if (val <= 0x3FFF) {
		buffer->w8(0x80 | (val >> 7));
		buffer->w8(val & 0x7F);
	}
	else if (val <= 0x1FFFFF) {
		buffer->w8(0x80 | (val >> 14));
		buffer->w8(0x80 | ((val >> 7) & 0x7F));
		buffer->w8(val & 0x7F);
	}
	else {
		buffer->w8(0x80 | (val >> 22));
		buffer->w8(0x80 | ((val >> 15) & 0x7F));
		buffer->w8(0x80 | ((val >> 8) & 0x7F));
		buffer->w8(val & 0xFF);
	}
}
void HJFLVUtils::wu29bValue(HJBuffer::Ptr buffer, uint32_t val)
{
	wu29(buffer, 1 | ((val & 0xFFFFFFF) << 1));
}

void HJFLVUtils::w4cc(HJBuffer::Ptr buffer, int codecID)
{
	switch (codecID)
	{
	case AV_CODEC_ID_AV1: {
		buffer->w8('a');
		buffer->w8('v');
		buffer->w8('0');
		buffer->w8('1');
		break;
	}
	case AV_CODEC_ID_HEVC: {
		buffer->w8('h');
		buffer->w8('v');
		buffer->w8('c');
		buffer->w8('1');
		break;
	}
	case AV_CODEC_ID_H264: {
		buffer->w8('a');
		buffer->w8('v');
		buffer->w8('c');
		buffer->w8('1');
		break;
	}
	default:
		HJLogw(HJFMT("not support codecID:{}", codecID));
		break;
	}
}

HJBuffer::Ptr HJFLVUtils::buildAdditionalAudioData(HJFLVPacket::Ptr packet, bool isHeader, size_t index)
{
	if (!packet || !packet->m_data) {
		return nullptr;
	}
	HJBuffer::Ptr es = packet->m_data;
	if (!es->data() || !es->size()) {
		return nullptr;
	}
	HJBuffer::Ptr buffer = std::make_shared<HJBuffer>(es->size() + HJ_FLV_TAG_SIZE_DEFAULT);

	buffer->w8(AMF_STRING);
	buffer->wstring("additionalMedia");

	buffer->w8(AMF_OBJECT);
	{
		buffer->wstring("id");

		buffer->w8(AMF_STRING);
		buffer->wstring("stream0");

		/* ----- */
		buffer->wstring("media");

		buffer->w8(AMF_AVMPLUS);
		buffer->w8(AMF3_BYTE_ARRAY);
		wu29bValue(buffer, (uint32_t)es->size() + 2);
		buffer->w8(0xaf);
		buffer->w8(isHeader ? 0 : 1);
		buffer->write(es->data(), es->size());
	}
	buffer->wb24(AMF_OBJECT_END);

	return buffer;
}

HJBuffer::Ptr HJFLVUtils::buildAdditionalAudioTag(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader, size_t index)
{
	HJBuffer::Ptr audioData = buildAdditionalAudioData(packet, isHeader, index);
	if (!audioData) {
		return nullptr;
	}
	int32_t time_ms = packet->m_dts - dts0ffset;
	HJBuffer::Ptr tag = std::make_shared<HJBuffer>(audioData->size() + HJ_FLV_TAG_SIZE_DEFAULT);

	//HJFLogi("additional audio:{}", time_ms);

	tag->w8(RTMP_PACKET_TYPE_INFO); //18

	tag->wb24((uint32_t)audioData->size());
	tag->wb24((uint32_t)time_ms);
	tag->w8((time_ms >> 24) & 0x7F);
	tag->wb24(0);

	tag->write(audioData->data(), audioData->size());

	tag->wb32((uint32_t)tag->size());

	return tag;
}

HJBuffer::Ptr HJFLVUtils::buildVideoTag(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader)
{
	if (!packet) {
		HJLoge("error, packet have no video data");
		return nullptr;
	}
	HJBuffer::Ptr es = packet->m_data;
	//if (!es->data() || !es->size()) {
	//	return nullptr;
	//}
	size_t esDataSize = es ? es->size() : 0;
	HJBuffer::Ptr tag = std::make_shared<HJBuffer>(esDataSize + HJ_FLV_TAG_SIZE_DEFAULT);
	int64_t ct_offset_ms = packet->m_pts - packet->m_dts;
	int32_t time_ms = packet->m_dts - dts0ffset;

	//HJFLogi("build tag video - time_ms:{}, offset:{}, dts0ffset:{}", time_ms, ct_offset_ms, dts0ffset);
	tag->w8(RTMP_PACKET_TYPE_VIDEO);

	tag->wb24((uint32_t)esDataSize + 5);
	tag->wb24((uint32_t)time_ms);
	tag->w8((time_ms >> 24) & 0x7F);
	tag->wb24(0);

	/* these are the 5 extra bytes mentioned above */
	if (packet->m_codecid == AV_CODEC_ID_H265) {
		tag->w8(packet->m_keyFrame ? 0x1c : 0x2c);
	}else {
		tag->w8(packet->m_keyFrame ? 0x17 : 0x27);
	}
	tag->w8(isHeader ? 0 : 1);
	tag->wb24(ct_offset_ms);
	if (es) {
		tag->write(es->data(), es->size());
	}

	/* write tag size (starting byte doesn't count) */
	tag->wb32((uint32_t)tag->size());

	tag->setTimestamp(time_ms);
	tag->setName(AVCodecIDToString((AVCodecID)packet->m_codecid));

	return tag;
}

HJBuffer::Ptr HJFLVUtils::buildPacket(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader)
{
	if (HJMEDIA_TYPE_VIDEO == packet->m_type) {
		return buildVideoTag(packet, dts0ffset, isHeader);
	} else {
		return buildAudioTag(packet, dts0ffset, isHeader);
	}
}

HJBuffer::Ptr HJFLVUtils::buildAdditionalPacket(HJFLVPacket::Ptr packet, int64_t dts0ffset, bool isHeader, size_t index)
{
	if (HJMEDIA_TYPE_VIDEO == packet->m_type) {
		HJLogw("additional not support video");
		return nullptr;
	}
	return buildAdditionalAudioTag(packet, dts0ffset, isHeader, index);
}

/**
* https://veovera.org/docs/enhanced/enhanced-rtmp-v1.html
* https://github.com/veovera/enhanced-rtmp
*/
HJBuffer::Ptr HJFLVUtils::buildPacketEx(HJFLVPacket::Ptr packet, int64_t dts0ffset, int codecID, int type)
{
	if (!packet) {
		return nullptr;
	}
	HJBuffer::Ptr es = packet->m_data;
	//if (!es->data() || !es->size()) {
	//	return nullptr;
	//}
	size_t esDataSize = es ? es->size() : 0;
	HJBuffer::Ptr tag = std::make_shared<HJBuffer>(esDataSize + HJ_FLV_TAG_SIZE_DEFAULT);
	int64_t offset = packet->m_pts - packet->m_dts;
	int32_t time_ms = packet->m_dts - dts0ffset;

	// packet head
	int header_metadata_size = 5;
	// 3 extra bytes for composition time offset
	if (codecID == AV_CODEC_ID_HEVC && type == HJFLV_PACKETTYPE_FRAMES) {
		header_metadata_size = 8;
	}
	tag->w8(RTMP_PACKET_TYPE_VIDEO);
	tag->wb24((uint32_t)esDataSize + header_metadata_size);
	tag->wtimestamp(time_ms);
	tag->wb24(0); // always 0

	// packet ext header
	tag->w8(HJ_FRAME_HEADER_EX | type | (packet->m_keyFrame ? HJFLV_FT_KEY : HJFLV_FT_INTER));
	w4cc(tag, codecID);

	// hevc composition time offset
	if (codecID == AV_CODEC_ID_HEVC && type == HJFLV_PACKETTYPE_FRAMES) {
		tag->wb24(offset);
	}

	// packet data
	if (es) {
		tag->write(es->data(), es->size());
	}

	// packet tail
	tag->wb32((uint32_t)tag->size());

	return tag;
}

HJBuffer::Ptr HJFLVUtils::buildPacketStart(HJFLVPacket::Ptr packet, int codecID)
{
	return buildPacketEx(packet, 0, codecID, HJFLV_PACKETTYPE_SEQ_START);
}
HJBuffer::Ptr HJFLVUtils::buildPacketFrames(HJFLVPacket::Ptr packet, int codecID, int64_t dts0ffset)
{
	int packet_type = HJFLV_PACKETTYPE_FRAMES;
	// PACKETTYPE_FRAMESX is an optimization to avoid sending composition
	// time offsets of 0. See Enhanced RTMP spec.
	if (codecID == AV_CODEC_ID_HEVC && packet->m_dts == packet->m_pts)
		packet_type = HJFLV_PACKETTYPE_FRAMESX;

	return buildPacketEx(packet, dts0ffset, codecID, packet_type);
}

HJBuffer::Ptr HJFLVUtils::buildPacketEnd(HJFLVPacket::Ptr packet, int codecID)
{
	return buildPacketEx(packet, 0, codecID, HJFLV_PACKETTYPE_SEQ_END);
}

HJBuffer::Ptr HJFLVUtils::buildPacketMetadata(int codecID, int bits_per_raw_sample,
	uint8_t color_primaries, int color_trc,
	int color_space, int min_luminance,
	int max_luminance)
{
	HJBuffer::Ptr metadata = std::make_shared<HJBuffer>(HJ_FLV_META_DATA_SIZE_DEFAULT);
	// metadata data array
	{
		metadata->w8(HJFLV_DATA_TYPE_STRING);
		metadata->wstring("colorInfo");
		metadata->w8(HJFLV_DATA_TYPE_OBJECT);
		{
			// colorConfig:
			metadata->wstring("colorConfig");
			metadata->w8(HJFLV_DATA_TYPE_OBJECT);
			{
				metadata->wstring("bitDepth");
				metadata->w8(HJFLV_DATA_TYPE_NUMBER);
				metadata->wbd(bits_per_raw_sample);

				metadata->wstring("colorPrimaries");
				metadata->w8(HJFLV_DATA_TYPE_NUMBER);
				metadata->wbd(color_primaries);

				metadata->wstring("transferCharacteristics");
				metadata->w8(HJFLV_DATA_TYPE_NUMBER);
				metadata->wbd(color_trc);

				metadata->wstring("matrixCoefficients");
				metadata->w8(HJFLV_DATA_TYPE_NUMBER);
				metadata->wbd(color_space);
			}
			metadata->w8(0);
			metadata->w8(0);
			metadata->w8(HJFLV_DATA_TYPE_OBJECT_END);

			if (max_luminance != 0) {
				// hdrMdcv
				metadata->wstring("hdrMdcv");
				metadata->w8(HJFLV_DATA_TYPE_OBJECT);
				{
					metadata->wstring("maxLuminance");
					metadata->w8(HJFLV_DATA_TYPE_NUMBER);
					metadata->wbd(max_luminance);

					metadata->wstring("minLuminance");
					metadata->w8(HJFLV_DATA_TYPE_NUMBER);
					metadata->wbd(min_luminance);
				}
				metadata->w8(0);
				metadata->w8(0);
				metadata->w8(HJFLV_DATA_TYPE_OBJECT_END);
			}
		}
		metadata->w8(0);
		metadata->w8(0);
		metadata->w8(HJFLV_DATA_TYPE_OBJECT_END);
	}

	HJBuffer::Ptr tag = std::make_shared<HJBuffer>(HJ_FLV_META_DATA_SIZE_DEFAULT + HJ_FLV_TAG_SIZE_DEFAULT);
	// packet head
	tag->w8(RTMP_PACKET_TYPE_VIDEO);
	tag->wb24((uint32_t)metadata->size() + 5); // 5 = (w8+w4cc)
	tag->wtimestamp(0);
	tag->wb24(0); // always 0

	// packet ext header
	// these are the 5 extra bytes mentioned above
	tag->w8(HJ_FRAME_HEADER_EX | HJFLV_PACKETTYPE_METADATA);
	w4cc(tag, codecID);
	// packet data
	tag->write(metadata->data(), metadata->size());

	// packet tail
	tag->wb32((uint32_t)tag->size());

	return tag;
}

AVal* HJFLVUtils::flv_str(AVal* out, const char* str)
{
	out->av_val = (char*)str;
	out->av_len = (int)strlen(str);
	return out;
}
void HJFLVUtils::enc_num_val(char** enc, char* end, const char* name, double val)
{
	AVal s;
	*enc = AMF_EncodeNamedNumber(*enc, end, flv_str(&s, name), val);
}
void HJFLVUtils::enc_bool_val(char** enc, char* end, const char* name, bool val)
{
	AVal s;
	*enc = AMF_EncodeNamedBoolean(*enc, end, flv_str(&s, name), val);
}
void HJFLVUtils::enc_str_val(char** enc, char* end, const char* name, const char* val)
{
	AVal s1, s2;
	*enc = AMF_EncodeNamedString(*enc, end, flv_str(&s1, name), flv_str(&s2, val));
}
void HJFLVUtils::enc_str(char** enc, char* end, const char* str)
{
	AVal s;
	*enc = AMF_EncodeString(*enc, end, flv_str(&s, str));
}

size_t HJFLVUtils::getADTSHeaderLength(const uint8_t* data, size_t length) 
{
    // 检查数据有效性和最小长度（ADTS头至少7字节）
    if (data == nullptr || length < 7) {
        return 0;
    }
    
    // 验证同步字0xFFF（前12位必须为1）
    if ((data[0] != 0xFF) || ((data[1] & 0xF0) != 0xF0)) {
        return 0;
    }
    
    // 解析protection_absent位（第12位）
    bool protectionAbsent = (data[1] & 0x01);
    size_t adtsHeaderLength = protectionAbsent ? 7 : 9;
    
    // 检查数据长度是否足够容纳完整的ADTS头
    if (length < adtsHeaderLength) {
        return 0;
    }
    
    // 解析帧长度字段（共13位）
    uint32_t frameLength = ((static_cast<uint32_t>(data[3]) & 0x03) << 11) | 
                           (static_cast<uint32_t>(data[4]) << 3) | 
                           ((static_cast<uint32_t>(data[5]) & 0xE0) >> 5);
    
    // 验证帧长度合理性
    if (frameLength < adtsHeaderLength || frameLength > length) {
        return 0;
    }
    
    // 验证音频对象类型是否在有效范围内（1-4）
    uint8_t audioObjectType = (data[2] >> 6) + 1;
    if (audioObjectType < 1 || audioObjectType > 4) {
        return 0;
    }
    
    // 验证采样率索引是否在有效范围内（0-12）
    uint8_t samplingFreqIndex = (data[2] >> 2) & 0x0F;
    if (samplingFreqIndex > 12) {
        return 0;
    }
    
    // 所有验证通过，返回ADTS头长度
    return adtsHeaderLength;
}

NS_HJ_END