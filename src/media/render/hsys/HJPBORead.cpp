#include "HJPBORead.h"
#include "HJFLog.h"
#include "libyuv.h"
#include "HJSPBuffer.h"
#include "HJFileData.h"
#include "HJTime.h"

#define TEST_WRITE 0

NS_HJ_BEGIN

HJPBORead::HJPBORead()
{

}
HJPBORead::~HJPBORead()
{
	if (m_bInit)
	{
		glDeleteBuffers(2, m_pboIds);
	}
}
void HJPBORead::setReadCb(HJPBOReadCb i_cb)
{
	m_readCb = i_cb;
}
int HJPBORead::init(int i_nWidth, int i_nHeight)
{
	int i_err = 0;
	do
	{
		m_width = i_nWidth;
		m_height = i_nHeight;

		m_dataSize = m_width * m_height * 4;
		HJFLogi("width:{} height:{}", m_width, m_height);


		glGenBuffers(2, m_pboIds);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboIds[0]);
		glBufferData(GL_PIXEL_PACK_BUFFER, m_dataSize, 0, GL_STREAM_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboIds[1]);
		glBufferData(GL_PIXEL_PACK_BUFFER, m_dataSize, 0, GL_STREAM_READ);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		
		m_bInit = true;

	} while (false);
	return i_err;
}

void HJPBORead::priWrite(unsigned char *i_pBuffer, int i_width, int i_height) 
{
    static HJYuvWriter::Ptr yuvFilter = nullptr;

    if (!yuvFilter) {
        yuvFilter = HJYuvWriter::Create();
        yuvFilter->init("/data/storage/el2/base/haps/entry/files/test.yuv", i_width, i_height);
    }
    
    HJSPBuffer::Ptr rgbMirror = HJSPBuffer::create(i_width * i_height * 4);
    libyuv::ARGBCopy(i_pBuffer, i_width * 4, rgbMirror->getBuf(), i_width * 4, i_width, -i_height); //Y FLIP

    HJSPBuffer::Ptr yuvBuffer = HJSPBuffer::create(i_width * i_height * 3 / 2);
    libyuv::ABGRToI420(rgbMirror->getBuf(), i_width * 4, yuvBuffer->getBuf(), i_width,
                       yuvBuffer->getBuf() + i_width * i_height, i_width / 2,
                       yuvBuffer->getBuf() + i_width * i_height * 5 / 4, i_width / 2,
                       i_width, i_height);
    
    
    yuvFilter->write(yuvBuffer->getBuf());
}

int HJPBORead::read()
{
	int i_err = HJ_OK;
    int64_t t0, t1, t2, t3, t4, t5 = 0;
    t0 = HJCurrentSteadyMS();
	do
	{ 
		m_index = (m_index + 1) % 2;
		int nextIndex = (m_index + 1) % 2;

		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboIds[m_index]);
        t1 = HJCurrentSteadyMS();
		glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		if (!m_bValid)
		{
			m_bValid = true; // n-1 is not used;
			HJFLogi("first is not used would block");
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			i_err = HJ_WOULD_BLOCK;
			break;
		}

		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboIds[nextIndex]);
        
        t2 = HJCurrentSteadyMS();
		GLubyte* src = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_dataSize, GL_MAP_READ_BIT);
        
        t3 = HJCurrentSteadyMS();
		if (src)
		{
#if TEST_WRITE
            priWrite((unsigned char*)src, m_width, m_height);
#endif
            t4 = HJCurrentSteadyMS();
			if (m_readCb)
			{
				m_readCb((unsigned char*)src, m_width, m_height);
			}
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);        // release pointer to the mapped buffer
			i_err = HJ_OK;
		}
		else
		{
			HJFLogi("not find src error");
		}
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	} while (false);
    t5 = HJCurrentSteadyMS();
    //HJFLogi("PBO Read timediff:{} {} {} {} {} {}", (t5 - t0), (t1 - t0), (t2 - t1), (t3 - t2), (t4 - t3), (t5 - t4));
	return i_err;
}

NS_HJ_END

