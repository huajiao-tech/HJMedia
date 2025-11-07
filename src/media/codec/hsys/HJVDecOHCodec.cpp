#include "HJVDecOHCodec.h"
#include "HJComUtils.h"
#include "HJFLog.h"
#include "multimedia/player_framework/native_avcodec_videodecoder.h"
#include "HJFFHeaders.h"
#include "HJOGRenderWindowBridge.h"
#include <native_window/external_window.h>
#include "HJBSFParser.h"
#include "HJTime.h"
#include "HJH2645Parser.h"

NS_HJ_BEGIN

HJVDecOHCodec::HJVDecOHCodec()
{
    //m_timeBase = {1, 1000};
}
HJVDecOHCodec::~HJVDecOHCodec()
{
	priDone();
	HJFLogi("{} ~HOVideoHWDecoder", getName());
}
int HJVDecOHCodec::init(const HJStreamInfo::Ptr& info)
{
	HJFLogi("{} init entry this:{}", getName(), size_t(this));
	OH_AVFormat* format = nullptr;
	int i_err = HJ_OK;
	do
	{
		HJOGRenderWindowBridge::Ptr bridge = nullptr;
		if (!info->haveStorage(HJ_CatchName(HJOGRenderWindowBridge::Ptr)))
		{
			i_err = HJErrCodecInit;
			break;
		}
		bridge = info->getValue<HJOGRenderWindowBridge::Ptr>(HJ_CatchName(HJOGRenderWindowBridge::Ptr));
		if (!bridge)
		{
			i_err = HJErrCodecInit;
			HJFLoge("{} bridge error", getName());
			break;
		}

        m_inputBufferListener = info->getValue<HJRunnable>("onNeedInputBuffer");
        m_outputBufferListener = info->getValue<HJRunnable>("onNewOutputBuffer");
        
		i_err = HJBaseCodec::init(info);
		if (HJ_OK != i_err)
		{
			break;
		}
		HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);

		HJHWDevice::Ptr device = std::make_shared<HJHWDevice>(HJDEVICE_TYPE_OHCODEC);
		videoInfo->setHWDevice(device);

		AVCodecParameters* codecParam = (AVCodecParameters*)videoInfo->getAVCodecParams();
		if (!codecParam)
		{
			HJFLoge("{} can't find codec params error", getName());
			i_err = HJErrInvalidParams;
			break;
		}

		m_codecID = (AV_CODEC_ID_NONE != codecParam->codec_id) ? codecParam->codec_id : videoInfo->getCodecID();

		std::string mime = "";
		if (m_codecID == AV_CODEC_ID_H264)
		{
			mime = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
		}
		else if (m_codecID == AV_CODEC_ID_H265)
		{
			mime = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
		}
		else
		{
			HJFLoge("{} id is not support:{}", getName(), (int)m_codecID);
			i_err = HJErrCodecInit;
			break;
		}
        HJFLogi("{} hw codec is:{} ", getName(), m_codecID == AV_CODEC_ID_H264 ? "H.264" : "H.265");
		m_decoder = OH_VideoDecoder_CreateByMime(mime.c_str());
		if (!m_decoder)
		{
			HJFLoge("{} create by mime error", getName());
			i_err = HJErrCodecInit;
			break;
		}

		format = OH_AVFormat_Create();
		if (!format)
		{
			i_err = HJErrCodecInit;
			HJFLoge("{} format return error", getName());
			break;
		}

		m_headerParser = std::make_shared<HJH2645Parser>();
		i_err = m_headerParser->init(codecParam);
		if (i_err < 0)
		{
			break;
		}
		HJSizei sizei = m_headerParser->getVSize();
		HJFLogi("{} old <w:{} h:{}  new <w:{}, h:{}> this:{} decoder:{}", getName(), videoInfo->m_width, videoInfo->m_height, sizei.w, sizei.h, size_t(this), size_t(m_decoder));
		videoInfo->m_width = sizei.w;
		videoInfo->m_height = sizei.h;

		OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, videoInfo->m_width);
		OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, videoInfo->m_height);

		//fixme after lfs the value is copy demo;
		OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, 30.0);
		OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

		i_err = OH_VideoDecoder_Configure(m_decoder, format);
		if (i_err != AV_ERR_OK)
		{
			HJFLoge("{} configure error", getName());
			i_err = HJErrCodecInit;
			break;
		}

		OHNativeWindow* nativeWindow = bridge->getNativeWindow();
		i_err = OH_VideoDecoder_SetSurface(m_decoder, nativeWindow);
		if (i_err != AV_ERR_OK)
		{
			HJFLoge("{} configure error: {}", getName(), i_err);
			i_err = -1;
			break;
		}


		//set w h
		i_err = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, videoInfo->m_width, videoInfo->m_height);
		if (i_err != 0)
		{
			HJFLoge("{} OH_NativeWindow_NativeWindowHandleOpt error ", getName());
			i_err = HJErrCodecInit;
			break;
		}

		i_err = OH_VideoDecoder_RegisterCallback(m_decoder, { OnCodecError, OnCodecFormatChange, OnNeedInputBuffer, OnNewOutputBuffer }, this);
		if (i_err != AV_ERR_OK)
		{
			HJFLoge("{} OH_VideoDecoder_RegisterCallback error: {}", getName(), i_err);
			i_err = HJErrCodecInit;
			break;
		}
		i_err = OH_VideoDecoder_Prepare(m_decoder);
		if (i_err != AV_ERR_OK)
		{
			HJFLoge("{} OH_VideoDecoder_Prepare error: {}", getName(), i_err);
			i_err = HJErrCodecInit;
			break;
		}
		//m_stat.m_prepareTime = MTMCurrentSteadyMS();
		i_err = OH_VideoDecoder_Start(m_decoder);
		if (i_err != AV_ERR_OK)
		{
			HJFLoge("{} OH_VideoDecoder_Start error: {}", getName(), i_err);
			i_err = HJErrCodecInit;
			break;
		}
		m_parser = std::make_shared<HJBSFParser>();
		i_err = m_parser->init("", codecParam);
		if (HJ_OK != i_err)
		{
			HJFLoge("{} bsf parser init error", getName());
			break;
		}

		m_state = HJVDecOHCodecState_ready;
	} while (false);

	if (format)
	{
		OH_AVFormat_Destroy(format);
		format = nullptr;
	}
	HJFLogi("{} init end i_err:{}", getName(), i_err);
	return i_err;
}
void HJVDecOHCodec::priRenderOutCache(int64_t i_key, uint32_t i_idx)
{
	HJ_LOCK(m_mutex);
	m_map[i_key] = i_idx;
}
int HJVDecOHCodec::renderOutput(bool i_bRender, int64_t i_outputKey)
{
	return priRenderOutput(i_bRender, i_outputKey);
}
int HJVDecOHCodec::priRenderOutput(bool i_bRender, int64_t i_outputKey)
{
	int i_err = 0;
	HJ_LOCK(m_mutex);
	do
	{
		if (m_map.find(i_outputKey) != m_map.end())
		{
			uint32_t idx = m_map[i_outputKey];
			//JFLogi("{} priRenderOutput idx {}", m_insName, idx);
			int ret = 0;
			if (i_bRender)
			{
				ret = OH_VideoDecoder_RenderOutputBuffer(m_decoder, idx);
			}
			else
			{
				ret = OH_VideoDecoder_FreeOutputBuffer(m_decoder, idx);
			}
			m_map.erase(i_outputKey);
			if (ret < 0)
			{
				HJFLoge("{} OH_VideoDecoder_RenderOutputBuffer ret:{} key:{} idx:{}", getName(), ret, i_outputKey, idx);
			}
		}
		else
		{
			HJFLogi("{} not find output key, because the flush key is {}", getName(), i_outputKey);
		}
        m_mapSize = m_map.size();
	} while (false);
	return i_err;
}

int HJVDecOHCodec::getFrame(HJMediaFrame::Ptr& frame)
{
	int i_err = HJ_OK;
	do
	{
		if (m_state == HJVDecOHCodecState_outEOF)
		{
			i_err = HJ_EOF;
			HJFLogd("{} output is eof, not proc, direct return HJ_EOF", getName());
			break;
		}

		if (m_outputQueue.isEmpty())
		{
			i_err = HJ_WOULD_BLOCK;
			break;
		}

		HJVDecOHCodecInfo::Ptr outData = m_outputQueue.pop();

		OH_AVCodecBufferAttr attr;
		OH_AVErrCode ret = OH_AVBuffer_GetBufferAttr(outData->m_buffer, &attr);
		if (ret != AV_ERR_OK)
		{
			i_err = HJErrCodecDecode;
			HJFLoge("{} OH_AVBuffer_GetBufferAttr idx:{}", getName(), outData->m_index);
			break;
		}

		if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS)
		{
			frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_VIDEO);
			HJFLogi("{} output eof, return HJ_EOF", getName());
			m_state = HJVDecOHCodecState_outEOF;
			i_err = HJ_EOF;
			break;
		}

		HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);

		HJMediaFrame::Ptr mvf = HJMediaFrame::makeVideoFrame(videoInfo);

		mvf->setTimeBase(HJ_TIMEBASE_MS);
		int64_t timestamp = attr.pts / 1000;
		mvf->setPTS(timestamp);
		mvf->setDTS(timestamp);

		priRenderOutCache(m_outputKey, outData->m_index);

#if 0
		std::weak_ptr<HJVDecOHCodec> wtr = sharedFrom(this);
		HJMediaFrameListener listener = ([wtr, key = m_outputKey](const HJMediaFrame::Ptr frame)
			{
				int i_err = HJ_OK;
				do
				{
					HJVDecOHCodec::Ptr ptr = wtr.lock();
					if (ptr)
					{
						bool bRender = true;
						if (frame->haveStorage("isOHCodecRender"))
						{
							bRender = frame->getValue<bool>("isOHCodecRender");
						}
						ptr->priRenderOutput(bRender, key);
					}
					else
					{
						HJFLogi("have destroy, not proc key:{}", key);
					}
				} while (false);
				return i_err;
			});
		(*mvf)["HJMediaFrameListener"] = (HJMediaFrameListener)listener;
#endif
		(*mvf)[HJ_CatchName(HJVDecOHCodecResObj::Ptr)] = (HJVDecOHCodecResObj::Ptr)(std::make_shared<HJVDecOHCodecResObj>(sharedFrom(this), m_outputKey));

		m_outputKey++;

		frame = std::move(mvf);
	} while (false);
	return i_err;
}
bool HJVDecOHCodec::isCanInput()
{
	return !m_inputQueue.isEmpty();
}
int HJVDecOHCodec::run(const HJMediaFrame::Ptr frame)
{
	int i_err = HJ_OK;
	do
	{
		if (m_state == HJVDecOHCodecState_inEOF)
		{
			i_err = HJ_WOULD_BLOCK;
			HJFLogd("{} input is eof, not proc, direct return wouldblock", getName());
			break;
		}
        
        if (m_inputQueue.isEmpty())
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }

		AVPacket* pkt = NULL;
		if (frame && (HJFRAME_EOF != frame->getFrameType()))
		{
			//frame->setAVTimeBase(m_timeBase);
			pkt = frame->getAVPacket();
			if (!pkt)
			{
				HJFLoge("{} run get avpacket error", getName());
				return HJErrInvalidParams;
			}
            m_regularLog.proc([this, pkt]()
            {
                HJFLogi("{} hwcodec run, inputSize:{} outputSize:{} mapSize:{} pts:{} dts:{} size:{}", getName(), m_inputQueue.size(), m_outputQueue.size(), m_mapSize, pkt->pts, pkt->dts, pkt->size);
            });
		}
		else
		{
			m_state = HJVDecOHCodecState_inEOF;
			HJFLogi("{} run eof enter", getName());
		}

		HJVDecOHCodecInfo::Ptr inData = m_inputQueue.pop();
		OH_AVCodecBufferAttr attr;
		OH_AVBuffer_GetBufferAttr(inData->m_buffer, &attr);
		attr.offset = 0;
		HJBuffer::Ptr outPKT = nullptr;
        
        int64_t statPts = 0;
        int statSize = 0;
		if (m_state == HJVDecOHCodecState_inEOF)
		{
			attr.pts = 0;
			attr.size = 0;
			attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
			HJFLogi("{} input flags add EOS frame", getName());
            
            statPts = attr.pts;
            statSize = attr.size;
		}
		else
		{
			HJBuffer::Ptr inPKTBuf = std::make_shared<HJBuffer>(pkt->data, pkt->size);
			//parse avccheader to longheader
			outPKT = m_parser->filter(inPKTBuf);
			if (!outPKT)
			{
				i_err = HJErrCodecDecode;
				HJFLoge("{} parser filter error", getName());
				break;
			}
			attr.pts = frame->getPTS() * 1000; //us
			attr.size = outPKT->size();
            
            statPts = attr.pts;
            statSize = attr.size;
		}
		OH_AVBuffer_SetBufferAttr(inData->m_buffer, &attr);

		if (m_state != HJVDecOHCodecState_inEOF)
		{
			//HJFLogi("{} testIsMatch OH_VideoDecoder_PushInputBuffer idx:{} pts:{} dts:{} dtsdiff:{}, size:{} isKey:{}", getName(), inData->m_index, frame->getPTS(), frame->getDTS(), (frame->getDTS()-m_lastdts),pkt->size, frame->isKeyFrame());
			//m_lastdts = frame->getDTS();

			uint8_t* source = OH_AVBuffer_GetAddr(inData->m_buffer);
			if (!source)
			{
				i_err = HJErrCodecDecode;
				HJFLoge("{} Get Addr error idx:{}", getName(), inData->m_index);
				break;
			}
			int32_t capacity = OH_AVBuffer_GetCapacity(inData->m_buffer);
			if (outPKT->size() > capacity)
			{
				i_err = HJErrCodecDecode;
				HJFLoge("{} size:{} > capacity:{} error", getName(), outPKT->size(), capacity);
				break;
			}
			memcpy(source, outPKT->data(), outPKT->size());
		}

		i_err = OH_VideoDecoder_PushInputBuffer(m_decoder, inData->m_index);
		if (i_err != AV_ERR_OK)
		{
			HJFLoge("{} OH_VideoDecoder_PushInputBuffer i_err:{} error idx:{} m_state:{} pts:{} size:{}", getName(), i_err, inData->m_index, (int)m_state, statPts, statSize);
			i_err = HJErrCodecDecode;
			break;
		}
	} while (false);
	return i_err;
}
void HJVDecOHCodec::priClearMapOnly()
{
	HJ_LOCK(m_mutex);
	m_map.clear();
}

void HJVDecOHCodec::priDone()
{
	if (!m_bDone)
	{
        m_bDone = true;
		HJFLogi("{} close enter ", getName());
		if (m_decoder)
		{
			m_inputQueue.clear();
			m_outputQueue.clear();
			HJFLogi("{} priFlushAll enter this:{} decoder:{}", getName(), size_t(this), size_t(m_decoder));
			priClearMapOnly();


			HJFLogi("{} OH_VideoDecoder_Flush enter", getName());
			//        Clear the input and output data buffered in the decoder. After this interface is called, all the Buffer
			//        * indexes previously reported through the asynchronous callback will be invalidated, make sure not to access
			//        * the Buffers corresponding to these indexes.
			OH_AVErrCode i_err = OH_VideoDecoder_Flush(m_decoder);
			if (i_err != AV_ERR_OK)
			{
				HJFLoge("{} OH_VideoDecoder_Flush error i_err:{}", getName(), i_err);
			}

			HJFLogi("{} OH_VideoDecoder_Stop enter", getName());
			i_err = OH_VideoDecoder_Stop(m_decoder);
			if (i_err != AV_ERR_OK)
			{
				HJFLoge("{} OH_VideoDecoder_Stop error i_err:{}", getName(), i_err);
			}

			HJFLogi("{} OH_VideoDecoder_Destroy enter", getName());
			i_err = OH_VideoDecoder_Destroy(m_decoder);
			if (i_err != AV_ERR_OK)
			{
				HJFLoge("{} OH_VideoDecoder_Destroy error i_err:{}", getName(), i_err);
			}

			//the close thread is equal, inputproc outputproc thread, so close not proc input and output proc; so m_map is not have value
			//must clear, because the close proc is slow;
			m_inputQueue.clear();
			m_outputQueue.clear();
			priClearMapOnly();

			m_state = HJVDecOHCodecState_idle;

			m_decoder = nullptr;
		}

		if (m_headerParser)
		{
			m_headerParser->done();
			m_headerParser = nullptr;
		}
		HJFLogi("{} close end ", getName());
	}
}
void HJVDecOHCodec::done()
{
	priDone();
}

void HJVDecOHCodec::priOnCodecError(OH_AVCodec* codec, int32_t errorCodea)
{
	HJFLoge("{} kernal codec error {}", getName(), errorCodea);
}
void HJVDecOHCodec::priOnCodecFormatChange(OH_AVCodec* codec, OH_AVFormat* formata)
{
	int width = 0;
	int height = 0;
	OH_AVFormat_GetIntValue(formata, OH_MD_KEY_WIDTH, &width);
	OH_AVFormat_GetIntValue(formata, OH_MD_KEY_HEIGHT, &height);
	HJFLogi("{} kernal format change w:{} h:{}", getName(), width, height);
}
void HJVDecOHCodec::priOnNeedInputBuffer(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer)
{
	HJVDecOHCodecInfo::Ptr data = HJVDecOHCodecInfo::Create(index, buffer);
	m_inputQueue.push_back(data);
        
    if (m_inputBufferListener) {
        m_inputBufferListener();
    }
}
void HJVDecOHCodec::priOnNewOutputBuffer(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer)
{
	HJVDecOHCodecInfo::Ptr data = HJVDecOHCodecInfo::Create(index, buffer);
	m_outputQueue.push_back(data);
    
    if (m_outputBufferListener) {
        m_outputBufferListener();
    }
}

void HJVDecOHCodec::OnCodecError(OH_AVCodec* codec, int32_t errorCode, void* userData)
{
	HJVDecOHCodec* the = static_cast<HJVDecOHCodec*>(userData);
	if (the)
	{
		the->priOnCodecError(codec, errorCode);
	}
}
void HJVDecOHCodec::OnCodecFormatChange(OH_AVCodec* codec, OH_AVFormat* format, void* userData)
{
	HJVDecOHCodec* the = static_cast<HJVDecOHCodec*>(userData);
	if (the)
	{
		the->priOnCodecFormatChange(codec, format);
	}
}
void HJVDecOHCodec::OnNeedInputBuffer(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer, void* userData)
{
	HJVDecOHCodec* the = static_cast<HJVDecOHCodec*>(userData);
	if (the)
	{
		//HJFLogi("{} OnNeedInputBuffer enter this:{} decoder:{} index:{}", the->getName(), size_t(the), size_t(codec), index);
		the->priOnNeedInputBuffer(codec, index, buffer);
		//HJFLogi("{} OnNeedInputBuffer end the:{} index:{}", the->getName(), size_t(the), index);    
	}
}
void HJVDecOHCodec::OnNewOutputBuffer(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer, void* userData)
{
	HJVDecOHCodec* the = static_cast<HJVDecOHCodec*>(userData);
	if (the)
	{
		//HJFLogi("{} OnNewOutputBuffer enter this:{} decoder:{} index:{}", the->getName(), size_t(the), size_t(codec), index);
		the->priOnNewOutputBuffer(codec, index, buffer);
		//HJFLogi("{} OnNewOutputBuffer end this:{} index:{}", the->getName(), size_t(the), index);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
HJVDecOHCodecResObj::~HJVDecOHCodecResObj()
{
	std::shared_ptr<HJVDecOHCodec> ptr = m_wtr.lock();
	if (ptr)
	{
		ptr->renderOutput(m_bRender, m_key);
	}
	else
	{
		HJFLogi("HJVDecOHCodecResObj destroy, not proc key:{}", m_key);
	}
}

NS_HJ_END