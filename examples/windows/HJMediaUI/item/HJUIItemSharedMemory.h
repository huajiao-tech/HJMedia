#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include "HJSPBuffer.h"

NS_HJ_BEGIN

class HJYuvReader;
class HJSPBuffer;
class HJYuvTexCvt;

class HJSharedMemoryConsumer;
class HJSharedMemoryProducer;

class HJUIItemSharedMemory : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemSharedMemory);
    
	HJUIItemSharedMemory();
	virtual ~HJUIItemSharedMemory();

	virtual int run() override;
    
private:
	int priProcProducer();
	int priProcConsumer();
	void priDraw();
	int priTryGetParam();
	void priDone();

	std::shared_ptr<HJSharedMemoryConsumer> m_consumer = nullptr;
	int m_cacheWidth = 0;
    int m_cacheHeight = 0;
	HJSPBuffer::Ptr m_pBuffer = nullptr;
	HJSPBuffer::Ptr m_pYUV420 = nullptr;

	std::shared_ptr<HJSharedMemoryProducer> m_producer = nullptr;

	std::shared_ptr<HJYuvTexCvt> m_yuvTexCvt = nullptr;

	std::shared_ptr<HJYuvReader> m_yuvReader = nullptr;

	std::string m_yuvFilePath = "E:/video/yuv/dance1_504_896.yuv";
	int m_width = 504;
	int m_height = 896;
};

NS_HJ_END



