#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"

NS_HJ_BEGIN

class HJPBORead;

class HJPBOReadWrapper
{
public:
	HJ_DEFINE_CREATE(HJPBOReadWrapper)
	HJPBOReadWrapper();
	virtual ~HJPBOReadWrapper();
    void setReadCb(HJMediaDataReaderCb i_cb)
    {
        m_cb = i_cb;
    }
	int process(int i_nWidth, int i_nHeight);
private:
    std::shared_ptr<HJPBORead> m_pboReader = nullptr;
    int m_width = 0;
    int m_height = 0;
    HJMediaDataReaderCb m_cb = nullptr;
    int m_idx = 0;
};

NS_HJ_END