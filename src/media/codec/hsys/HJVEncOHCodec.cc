//***********************************************************************************//
// HJMedia FRAMEWORK SOURCE;
// AUTHOR:
// CREATE TIME:
//***********************************************************************************//
#include "HJVEncOHCodec.h"
#include "HJFFUtils.h"
#include "HJTime.h"
#include "HJFLog.h"
#include "deviceinfo.h"
#include <dlfcn.h>

#define USE_ROI_20 1

NS_HJ_BEGIN

std::string HJVEncOHCodec::s_h264mime = "video/avc";
std::string HJVEncOHCodec::s_h265mime = "video/hevc";
std::string HJVEncOHCodec::s_roi_params_val = "";
//***********************************************************************************//
HJVEncOHCodec::HJVEncOHCodec()
{
	setName(HJMakeGlobalName(HJ_TYPE_NAME(HJVEncOHCodec)));
	m_codecID = AV_CODEC_ID_H264;
	m_timeBase = HJ_TIMEBASE_MS;
}

HJVEncOHCodec::~HJVEncOHCodec()
{
	HJFLogi("{} ~HJVEncOHCodec enter", m_name);
//	if (m_surfaceCb)
//	{
//		HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);
//		m_surfaceCb(m_nativeWindow, videoInfo->m_width, videoInfo->m_height, false);
//	}

	do
	{
		if (m_encoder)
		{
            
            if (m_roiCb)
	        {
                HJRoiInfoVectorPtr roiInfo = nullptr;
                m_roiCb(roiInfo, HJ_ROI_CB_FLAG_CLOSE);
            }
            
			int ret = OH_VideoEncoder_Flush(m_encoder);
			if (AV_ERR_OK != ret)
			{
				HJFLoge("OH_VideoEncoder_Flush error:{}", ret);
				break;
			}
			m_outputQueue.clear();
			ret = OH_VideoEncoder_Stop(m_encoder);
			if (AV_ERR_OK != ret)
			{
				HJFLoge("OH_VideoEncoder_Stop error:{}", ret);
				break;
			}
			ret = OH_VideoEncoder_Destroy(m_encoder);
			if (AV_ERR_OK != ret)
			{
				HJFLoge("OH_VideoEncoder_Destroy error:{}", ret);
				break;
			}
			m_encoder = nullptr;
		}
	} while (false);
	HJFLogi("{} ~HJVEncOHCodec end", m_name);
}

int HJVEncOHCodec::init(const HJStreamInfo::Ptr &info)
{
	HJFLogi("init begin");
	int res = HJBaseCodec::init(info);
	if (HJ_OK != res)
	{
		return res;
	}
	OH_AVFormat *format = nullptr;
	do
	{
		HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);
		if (AV_CODEC_ID_NONE != videoInfo->m_codecID)
		{
			m_codecID = videoInfo->m_codecID;
		}
		m_codecName = videoInfo->m_codecName;

		std::string mime = (m_codecID == AV_CODEC_ID_H265) ? s_h265mime : s_h264mime;

		m_encoder = OH_VideoEncoder_CreateByMime(mime.c_str());
		if (!m_encoder)
		{
			res = HJErrCodecCreate;
			HJFLoge("create mime error");
			break;
		}

		format = OH_AVFormat_Create();

		OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, videoInfo->m_width);
		OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, videoInfo->m_height);
		OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, videoInfo->m_frameRate);
		OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
		OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, BITRATE_MODE_CBR);
		OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, videoInfo->m_bitrate);
		OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, 1000 * videoInfo->m_gopSize / videoInfo->m_frameRate); // ms for example, (2 * 1000 / fps)
		OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, 0);
        HJFLogi("init begin w:{} h:{} fps:{} bitrate:{}", videoInfo->m_width, videoInfo->m_height, videoInfo->m_frameRate, videoInfo->m_bitrate);
        
        
		res = OH_VideoEncoder_Configure(m_encoder, format);
        OH_AVFormat_Destroy(format);
		if (res != AV_ERR_OK)
		{
			HJFLoge("OH_VideoEncoder_Configure error i_err:{}", res);
			res = HJErrCodecInit;
			break;
		}
		res = OH_VideoEncoder_GetSurface(m_encoder, &m_nativeWindow);
		if (res != AV_ERR_OK)
		{
			HJFLoge("OH_VideoEncoder_GetSurface error i_err:{}", res);
			res = HJErrCodecInit;
			break;
		}

		res = OH_NativeWindow_NativeWindowHandleOpt(m_nativeWindow, SET_BUFFER_GEOMETRY, videoInfo->m_width, videoInfo->m_height);
		if (res != AV_ERR_OK)
		{
			res = HJErrCodecInit;
			HJFLoge("OH_NativeWindow_NativeWindowHandleOpt set wh error w:{} h:{}", videoInfo->m_width, videoInfo->m_height);
			break;
		}

		res = OH_VideoEncoder_RegisterCallback(m_encoder, {OnCodecError, OnCodecFormatChange, OnNeedInputBuffer, OnNewOutputBuffer}, this);
		if (res != AV_ERR_OK)
		{
			HJFLoge("OH_VideoEncoder_RegisterCallback error i_err:{}", res);
			res = HJErrCodecInit;
			break;
		}

		res = OH_VideoEncoder_Prepare(m_encoder);
		if (res != AV_ERR_OK)
		{
			HJFLoge("OH_VideoEncoder_Prepare error i_err:{}", res);
			res = HJErrCodecInit;
			break;
		}

		res = priStart();
		if (res < 0)
		{
			HJFLoge("priStart res:{}", res);
			res = HJErrCodecInit;
			break;
		}

		m_newBufferCb = info->getValue<HJRunnable>("newBufferCb");
		m_runState = HJRun_Init;
		HJLogi("init end");
	} while (false);
	return res;
}

int HJVEncOHCodec::setEof()
{
	int i_err = HJ_OK;
	do
	{
		if (!m_encoder)
		{
			i_err = HJErrCodecEncode;
			HJFLoge("setEof m_encoder empty");
			break;
		}
		i_err = OH_VideoEncoder_NotifyEndOfStream(m_encoder);
		if (i_err != AV_ERR_OK) 
		{
			HJFLoge("OH_VideoEncoder_NotifyEndOfStream res:{}", i_err);
			i_err = HJErrCodecEncode;
			break;
		}
	} while (false);
	return i_err;
}

int HJVEncOHCodec::adjustBitrate(int i_newBitrate)
{
	return 0;
	int i_err = HJ_OK;
	HJFLogi("adjustBitrate newBitrate:{}", i_newBitrate);
	OH_AVFormat *format = nullptr;
	do
	{
		if (!m_encoder)
		{
			i_err = HJErrCodecEncode;
			HJFLoge("adjustBitrate m_encoder empty");
			break;
		}
		format = OH_AVFormat_Create();
		if (!format)
		{
			i_err = HJErrCodecEncode;
			break;
		}
     	OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, i_newBitrate);
    	i_err = OH_VideoEncoder_SetParameter(m_encoder, format);
		if (i_err != AV_ERR_OK) 
		{
			HJFLoge("OH_VideoEncoder_SetParameter res:{}", i_err);
			i_err = HJErrCodecEncode;
			break;
		}
	} while (false);
	if (format)
	{
		OH_AVFormat_Destroy(format);
	}
	return i_err;
}
int64_t HJVEncOHCodec::getEncIdx()
{
	return m_index;
}

int HJVEncOHCodec::priStart()
{
	int i_err = HJ_OK;
	do
	{
		if (!m_encoder)
		{
			i_err = HJErrCodecInit;
			HJFLoge("start error");
			break;
		}
		HJFLogi("start enter");
		i_err = OH_VideoEncoder_Start(m_encoder);
		if (i_err != AV_ERR_OK)
		{
			HJFLoge("start error i_err:{}", i_err);
			i_err = HJErrCodecInit;
			break;
		}
		HJFLogi("start success");

//		if (m_surfaceCb)
//		{
//			HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);
//			i_err = m_surfaceCb(m_nativeWindow, videoInfo->m_width, videoInfo->m_height, true);
//		}
	} while (false);
	return i_err;
}

NativeWindow *HJVEncOHCodec::getNativeWindow()
{
    return m_nativeWindow;
}

int HJVEncOHCodec::getFrame(HJMediaFrame::Ptr &frame)
{
	int i_err = HJ_OK;
    HJVEncOHCodecInfo::Ptr ptr = nullptr;
	do
	{
		if (m_bEof)
		{
			i_err = HJ_EOF;
			HJFLogi("{}, eof not proc", getName());
			break;
		}
		if (m_outputQueue.isEmpty())
		{
			i_err = HJ_WOULD_BLOCK;
			break;
		}
        
		ptr = m_outputQueue.getFirst();
		m_outputQueue.pop();
        
        //HJFLogi("{} getFrame outputQueue size:{}", m_name, m_outputQueue.size());
        
        if ((m_index % 360) == 0)
        {
            HJFLogi("{}, OHCodecStat encoder m_index:{} m_outputQueue.size({})", getName(), m_index, m_outputQueue.size());
        }
        m_index++;
        
		OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAGS_NONE};
		OH_AVBuffer_GetBufferAttr(ptr->m_buffer, &attr);
		uint8_t *source = OH_AVBuffer_GetAddr((OH_AVBuffer *)(ptr->m_buffer));
		if (attr.flags & AVCODEC_BUFFER_FLAGS_CODEC_DATA)
		{
			m_headerBuf = std::make_shared<HJBuffer>(source, attr.size);
            m_keyCodecParams = HJMediaFrame::makeHJAVCodecParam(m_info, m_headerBuf);
            i_err = HJ_WOULD_BLOCK;
			break;
		}
		else if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS)
		{
			frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_VIDEO);
			m_bEof = true;
            i_err = HJ_EOF;
            break;	
		}
		else
		{
			HJVideoInfo::Ptr info = std::make_shared<HJVideoInfo>();
			info->clone(m_info);
			info->m_dataType = HJDATA_TYPE_ES;

			uint8_t *data = nullptr;
			int data_size = 0;
            HJBuffer::Ptr keyBuf = nullptr;
			if (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME)
			{
				int nTotal = m_headerBuf->size() + attr.size;
				keyBuf = std::make_shared<HJBuffer>(nTotal);
				memcpy(keyBuf->data(), m_headerBuf->data(), m_headerBuf->size());
				memcpy(keyBuf->data() + m_headerBuf->size(), source, attr.size);
                keyBuf->setSize(nTotal);
				data = keyBuf->data();
				data_size = keyBuf->size();
                info->setCodecParams(m_keyCodecParams);
			}
			else
			{
				data = source;
				data_size = attr.size;
			}

			frame = HJMediaFrame::makeMediaFrameAsAVPacket(info, data, data_size, (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME), ptr->m_timestamp, ptr->m_timestamp, m_timeBase);
            frame->setExtraTS(ptr->m_timestamp);
		}
		

	} while (false);
    
    if (ptr && m_encoder)
    {
        int i_err = OH_VideoEncoder_FreeOutputBuffer(m_encoder, ptr->m_index);
        if (i_err != AV_ERR_OK)
        {
            i_err = -1;
            HJFLoge("OH_VideoEncoder_FreeOutputBuffer error");
        }
    }
	return i_err;
}

void HJVEncOHCodec::priOnCodecError(OH_AVCodec *codec, int32_t errorCodea)
{
	HJFLoge("codec error {}", errorCodea);
}
void HJVEncOHCodec::priOnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *formata)
{
}
void HJVEncOHCodec::priOnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer)
{
	HJVEncOHCodecInfo::Ptr data = std::make_shared<HJVEncOHCodecInfo>();
	data->m_buffer = buffer;
	data->m_index = index;
	data->m_timestamp = HJCurrentSteadyMS();
	// HJFLogi("onNewOutputBuffer idx:{} timestamp:{}", data->m_index, data->m_timestamp);
	m_outputQueue.push_back(data);

	if (m_newBufferCb != nullptr) {
		m_newBufferCb();
	}
}

void HJVEncOHCodec::OnCodecError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
	HJVEncOHCodec *the = static_cast<HJVEncOHCodec *>(userData);
	if (the)
	{
		the->priOnCodecError(codec, errorCode);
	}
}
void HJVEncOHCodec::OnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
	HJVEncOHCodec *the = static_cast<HJVEncOHCodec *>(userData);
	if (the)
	{
		the->priOnCodecFormatChange(codec, format);
	}
}
void HJVEncOHCodec::OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
}
void HJVEncOHCodec::OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
	HJVEncOHCodec *the = static_cast<HJVEncOHCodec *>(userData);
	if (the)
	{
		the->priOnNewOutputBuffer(codec, index, buffer);
	}
}

NS_HJ_END