#pragma once

#include "HJPrerequisites.h"
#include <memory>
#include "shared-memory-queue.h"
#include "HJSPBuffer.h"

NS_HJ_BEGIN

class HJMediaFrame;

typedef enum HJSLSharedMemoryRet {
	SHAREDMEM_RET_OK                 = 0,
	SHAREDMEM_RET_RESOLUTION_CHANGED = 1,
	SHAREDMEM_RET_NO_READY           = -1,
	SHAREDMEM_RET_INVALID_PARAM      = -2,

} HJSLSharedMemoryRet;

class HJSharedMemoryProducer
{
public:
	HJ_DEFINE_CREATE(HJSharedMemoryProducer);

	HJSharedMemoryProducer();
	virtual ~HJSharedMemoryProducer();

	int init(int i_width, int i_height, int i_fps);
	int write(std::shared_ptr<HJMediaFrame> i_frame);
	void done();

private:
	void priDone();

	video_queue_t* m_vq = nullptr;
	enum queue_state m_prevState = SHARED_QUEUE_STATE_INVALID;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint64_t m_interval = 0;

	HJSPBuffer::Ptr m_buffer = nullptr;
};

class HJSharedMemoryConsumer
{
public:
	HJ_DEFINE_CREATE(HJSharedMemoryConsumer);

	HJSharedMemoryConsumer();
	virtual ~HJSharedMemoryConsumer();

	int init();
	int read(int i_dataCapacity, unsigned char *o_data);
	void done();

	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }

private:
	void priDone();

	video_queue_t* m_vq = nullptr;
	enum queue_state m_prevState = SHARED_QUEUE_STATE_INVALID;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint64_t m_interval = 0;
};

NS_HJ_END