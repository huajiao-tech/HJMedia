#include "HJSharedMemory.h"
#include "tiny-nv12-scale.h"
#include "HJMediaUtils.h"
#include "HJMediaFrame.h"
#include "HJFFHeaders.h"
#include "HJTime.h"
#include "libyuv.h"

NS_HJ_BEGIN

HJSharedMemoryProducer::HJSharedMemoryProducer()
{

}
HJSharedMemoryProducer::~HJSharedMemoryProducer()
{
	priDone();
}

int HJSharedMemoryProducer::init(int i_width, int i_height, int i_fps)
{
	int i_err = SHAREDMEM_RET_OK;
	do
	{
		uint64_t interval = 10000000ULL / i_fps;
		m_vq = video_queue_create(i_width, i_height, interval);
		if (!m_vq)
		{
			i_err = SHAREDMEM_RET_NO_READY;
			break;
		}
	} while (false);
    return i_err;
}
int HJSharedMemoryProducer::write(std::shared_ptr<HJMediaFrame> i_frame)
{
	int i_err = SHAREDMEM_RET_OK;
	do
	{
		if (!m_vq)
		{
			i_err = SHAREDMEM_RET_NO_READY;
			break;
		}

		HJAVFrame::Ptr avFrame = i_frame->getMFrame();
		AVFrame* frame = avFrame->getAVFrame();
		//HJFLoge("video render frame: {}, {}, {}", frame->width, frame->height, frame->pts);
		
		int width = frame->width;
		int height = frame->height;
		if (m_width != width || m_height != height)
		{
			m_width = width;
            m_height = height;
			m_buffer = HJSPBuffer::create(m_width * m_height * 3 / 2);
		}

		libyuv::I420ToNV12(frame->data[0], frame->linesize[0],
			frame->data[1], frame->linesize[1],
			frame->data[2], frame->linesize[2],
			m_buffer->getBuf(), width,
			m_buffer->getBuf() + width * height, width,
			width, height);

		uint8_t* data[2] = { m_buffer->getBuf(), m_buffer->getBuf() + width * height };
		uint32_t linesize[2] = {width, width};
		uint64_t timestamp = HJCurrentSteadyMS();

		video_queue_write(m_vq, data, linesize, timestamp);
	} while (false);
    return i_err;
}
void HJSharedMemoryProducer::done()
{
	priDone();
}

void HJSharedMemoryProducer::priDone()
{
	if (m_vq)
	{
		video_queue_close(m_vq);
		m_vq = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
HJSharedMemoryConsumer::HJSharedMemoryConsumer()
{
}
HJSharedMemoryConsumer::~HJSharedMemoryConsumer()
{
	priDone();
}

int HJSharedMemoryConsumer::init()
{
	int i_err = SHAREDMEM_RET_OK;
	do
	{
		m_vq = video_queue_open();
		if (!m_vq)
		{
			i_err = SHAREDMEM_RET_NO_READY;
			break;
		}

		uint32_t new_obs_cx = 0;
		uint32_t new_obs_cy = 0;
		uint64_t new_obs_interval = 0;
		video_queue_get_info(m_vq, &new_obs_cx, &new_obs_cy, &new_obs_interval);

		
		m_width = new_obs_cx;
		m_height = new_obs_cy;
		m_interval = new_obs_interval;

	} while(false);
	return i_err;
}
int HJSharedMemoryConsumer::read(int i_dataCapacity, unsigned char* o_data)
{
	int i_err = SHAREDMEM_RET_OK;
	do
	{
		if (!m_vq)
		{
			m_vq = video_queue_open();
		}

		enum queue_state state = video_queue_state(m_vq);
		if (state != m_prevState)
		{
			m_prevState = state;
			if (state == SHARED_QUEUE_STATE_READY)
			{
				uint32_t new_obs_cx = 0;
				uint32_t new_obs_cy = 0;
				uint64_t new_obs_interval = 0;
				video_queue_get_info(m_vq, &new_obs_cx, &new_obs_cy, &new_obs_interval);

				if (new_obs_cx != m_width || new_obs_cy != m_height)
				{
					m_width = new_obs_cx;
					m_height = new_obs_cy;
					m_interval = new_obs_interval;
					i_err = SHAREDMEM_RET_RESOLUTION_CHANGED;
					break;
				}
			}
			else if (state == SHARED_QUEUE_STATE_STOPPING)
			{
				priDone();
				i_err = SHAREDMEM_RET_NO_READY;
				break;
			}
			else if (state == SHARED_QUEUE_STATE_INVALID)
			{
				i_err = SHAREDMEM_RET_NO_READY;
				break;
			}			
		}

		if (m_vq && state == SHARED_QUEUE_STATE_READY)
		{
			if ((m_width <= 0) || (m_height <= 0))
			{
				i_err = SHAREDMEM_RET_NO_READY;
				break;
			}
			if (i_dataCapacity < (m_width * m_height * 3 / 2))
			{
				i_err = SHAREDMEM_RET_INVALID_PARAM;
				break;
			}
			uint64_t temp = 0;
			nv12_scale_t scaler;
			scaler.src_cx = m_width;
			scaler.src_cy = m_height;
			scaler.dst_cx = m_width;
			scaler.dst_cy = m_height;
			scaler.format = TARGET_FORMAT_NV12;
			bool bRet = video_queue_read(m_vq, &scaler, o_data, &temp);
			if (!bRet)
			{
				priDone();
				i_err = SHAREDMEM_RET_NO_READY;
				break;
			}
		}
	} while (false);
	return i_err;
	
}
void HJSharedMemoryConsumer::done()
{
	priDone();
}

void HJSharedMemoryConsumer::priDone()
{
	if (m_vq)
	{
		video_queue_close(m_vq);
		m_vq = nullptr;
	}
}

NS_HJ_END