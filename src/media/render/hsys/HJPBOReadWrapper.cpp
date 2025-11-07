
#include "HJPBOReadWrapper.h"
#include "HJFLog.h"
#include "libyuv.h"
#include "HJSPBuffer.h"
#include "HJFileData.h"
#include "HJTime.h"
#include "HJPBORead.h"


NS_HJ_BEGIN

HJPBOReadWrapper::HJPBOReadWrapper()
{

}
HJPBOReadWrapper::~HJPBOReadWrapper()
{
//    if (m_pboReader)
//    {
//        m_pboReader = nullptr;
//    }    
    m_pboReader = nullptr;
}

int HJPBOReadWrapper::process(int i_nWidth, int i_nHeight)
{
	int i_err = 0;
	do
	{
        bool bCreate = false;
		if ((m_width != i_nWidth) || (m_height != i_nHeight))
        {
            bCreate = true;
            HJFLogi("process create pbo {}x{} new {}x{}", m_width, m_height, i_nWidth, i_nHeight);
            m_width = i_nWidth;
            m_height = i_nHeight;
        }    
        if (bCreate)
        {
            m_pboReader = HJPBORead::Create();
            m_pboReader->setReadCb([this](unsigned char *i_pBuffer, int i_width, int i_height)
            {
                int ret = HJ_OK;
                HJSPBuffer::Ptr rgbMirror = HJSPBuffer::create(i_width * i_height * 4);
                libyuv::ARGBCopy(i_pBuffer, i_width * 4, rgbMirror->getBuf(), i_width * 4, i_width, -i_height); //Y FLIP
                if (m_cb)
                {
                    //HJFLogi("GPUTORAM PBO idx:{} width:{} height:{}", m_idx++, i_width, i_height);
                    ret = m_cb(rgbMirror, i_width, i_height);
                }
                return ret;
            });
            i_err = m_pboReader->init(m_width, m_height);
            if (i_err < 0)
            {
                break;
            }    
        }
        if (m_pboReader)
        {
            i_err = m_pboReader->read();
            if (i_err < 0)
            {
                break;
            }    
        }
	} while (false);
	return i_err;
}


NS_HJ_END

