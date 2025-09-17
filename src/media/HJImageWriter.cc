//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <iostream>
#include <fstream>
#include "HJImageWriter.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJImageWriter::HJImageWriter()
{
}

HJImageWriter::~HJImageWriter()
{
	m_encoder = nullptr;
	m_converter = nullptr;
}

int HJImageWriter::init(const HJVideoInfo::Ptr& videoInfo)
{
	if (!videoInfo) {
		return HJErrNotAlready;
	}
	m_info = videoInfo;
	HJLogi("init entry");

	int res = HJ_OK;
	do
	{
		m_encoder = HJBaseCodec::createVEncoder();
		if (!m_encoder) {
			res = HJErrNewObj;
			break;
		}
		res = m_encoder->init(m_info);
		if (HJ_OK != res) {
			break;
		}
		m_converter = std::make_shared<HJVSwsConverter>(m_info);
		if (!m_converter) {
			res = HJErrNewObj;
			break;
		}
	} while (false);

	return res;
}

int HJImageWriter::initWithJPG(const int width, const int height, const float quality)
{
	int res = HJ_OK;
	do
	{
		HJVideoInfo::Ptr videoInfo = std::make_shared<HJVideoInfo>(width, height);
		if (!videoInfo) {
			res = HJErrNewObj;
			break;
		}
		videoInfo->m_codecID = AV_CODEC_ID_MJPEG;
		videoInfo->m_pixFmt = AV_PIX_FMT_YUV420P;
		videoInfo->m_quality = quality;
		(*videoInfo)["threads"] = 1;
		(*videoInfo)["quality"] = quality;
		//
		res = init(videoInfo);
	} while (false);

	return res;
}

int HJImageWriter::initWithPNG(const int width, const int height)
{
	int res = HJ_OK;
	do
	{
		HJVideoInfo::Ptr videoInfo = std::make_shared<HJVideoInfo>(width, height);
		if (!videoInfo) {
			res = HJErrNewObj;
			break;
		}
		videoInfo->m_codecID = AV_CODEC_ID_PNG;
		videoInfo->m_pixFmt = AV_PIX_FMT_RGBA; //AV_PIX_FMT_RGB24
		(*videoInfo)["threads"] = 1;
		//
		res = init(videoInfo);
	} while (false);

	return res;
}

int HJImageWriter::writeFrame(const HJMediaFrame::Ptr frame, const std::string& url)
{
	if (!frame || url.empty()) {
		return HJErrInvalidParams;
	}
	HJLogi("write frame entry, url:" + url);

	int res = HJ_OK;
	do
	{
		HJMediaFrame::Ptr mavf = frame;
		HJVideoInfo::Ptr videoInfo = frame->getVideoInfo();
		if (videoInfo->m_width != m_info->m_width ||
			videoInfo->m_height != m_info->m_height ||
			videoInfo->m_pixFmt != m_info->m_pixFmt ||
			!HJ_IS_PRECISION_DEFAULT(videoInfo->m_rotate)) {
			mavf = m_converter->convert2(frame);
		}
        int64_t pts = (m_frameIdx * HJ_TIMEBASE_MS.den) / (m_info->m_frameRate * HJ_TIMEBASE_MS.num);
		mavf->setPTSDTS(pts, pts);
		AVFrame* avf = (AVFrame *)mavf->getAVFrame();
		if (!avf) {
			return HJErrInvalidParams;
		}
		//avf->quality = m_info->m_quality;
		//
		res = m_encoder->run(mavf);
		if (HJ_OK != res) {
            HJLogi("error, encode image input media frame failed, res:" + HJ2STR(res));
			break;
		}
		HJMediaFrame::Ptr outFrame = nullptr;
		res = m_encoder->getFrame(outFrame);
//		while (HJ_WOUNLD_BLOCK == res && !outFrame) {
//			res = m_encoder->getFrame(outFrame);
//		}
		if (HJ_OK != res) {
			HJLogi("error, get frame failed");
			break;
		}
		if (outFrame) {
			AVPacket* pkt = (AVPacket *)outFrame->getAVPacket();
			if (!pkt) {
				HJLogi("error, get AVPacket failed");
				res = HJErrInvalidData;
				break;
			}
			std::ofstream bfile;
			bfile.open(url, std::ios::out | std::ios::binary);
			if (!bfile.is_open()) {
                HJLogi("error, open file:" + url + " failed");
				res = HJErrFFLoadUrl;
				break;
			}
			bfile.write((const char *)pkt->data, pkt->size);
			bfile.close();
			//
#if HJ_HAVE_TRACKER
			outFrame->addTrajectory(HJakeTrajectory());
#endif
			HJLogi("write image:" + outFrame->formatInfo(true));
		}
	} while (false);
    //
//    m_encoder->flush();
    m_frameIdx++;
    //HJLogi("writeFrame end, res:" + HJ2STR(res));
    
	return res;
}

NS_HJ_END
