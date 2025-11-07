#pragma once

#include "HJPrerequisites.h"

//#if defined(HarmonyOS)
#include <GLES3/gl3.h>
//#endif

NS_HJ_BEGIN

using HJPBOReadCb = std::function<int(unsigned char *i_buf, int i_width, int i_height)>;


class HJPBORead
{
public:
	HJ_DEFINE_CREATE(HJPBORead)
	HJPBORead();
	virtual ~HJPBORead();
	void setReadCb(HJPBOReadCb i_cb);
	int init(int i_nWidth, int i_nHeight);
	int read();
private:
    void priWrite(unsigned char *i_pBuffer, int i_width, int i_height);
	int m_width = 0;
	int m_height = 0;
	int m_dataSize = 0;

	GLuint m_pboIds[2];

	bool m_bInit = false;
	bool m_bValid = false;
	int  m_index = 0;
	HJPBOReadCb m_readCb = nullptr;
};

NS_HJ_END