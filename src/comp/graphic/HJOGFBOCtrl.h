#pragma once

#include "HJPrerequisites.h"
#include "HJOGUtils.h"
#include "HJComUtils.h"

NS_HJ_BEGIN

class HJOGFBOCtrl : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJOGFBOCtrl)
    
	HJOGFBOCtrl();
	virtual ~HJOGFBOCtrl();
	
	int init(int i_nWidth, int i_nHeight, bool i_bTransparency = true);
    void done();
	GLuint attach();
	void detach();
	int getWidth();
	int getHeight();
	bool isTransparency();
	GLuint getTextureId();
    float *getMatrix()
    {
        return m_matrix;
    }
private:
    float m_matrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; 
	int m_width = 0;
	int m_height = 0;	
	bool m_bTransparency = true;
	bool m_bInit = false;

	GLuint m_framebuffer = 0;
	GLuint m_texture = 0;

	GLint m_preFramebuffer = 0;

	thread_local static std::atomic<int64_t> m_fboCreateCnt;
    thread_local static std::atomic<int64_t> m_fboDestroyCnt;
};

NS_HJ_END