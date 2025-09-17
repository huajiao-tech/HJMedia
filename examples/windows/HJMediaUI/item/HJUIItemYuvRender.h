#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;

NS_HJ_BEGIN

class HJYuvReader;
class HJSPBuffer;
class HJYuvTexCvt;

class HJUIItemYuvRender : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemYuvRender);
    
	HJUIItemYuvRender();
	virtual ~HJUIItemYuvRender();

	virtual int run() override;
    
private:
	int priTryGetParam();
	void priDone();

	std::shared_ptr<HJYuvTexCvt> m_yuvTexCvt = nullptr;

	std::shared_ptr<HJYuvReader> m_yuvReader = nullptr;

	std::string m_yuvFilePath = "E:/video/yuv/dance1_504_896.yuv";
	int m_width = 504;
	int m_height = 896;
};

NS_HJ_END



