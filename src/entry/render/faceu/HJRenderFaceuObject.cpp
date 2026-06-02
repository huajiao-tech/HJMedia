#include "HJRenderFaceuObject.h"
#include "HJError.h"
#include "HJFLog.h"
#include "HJRteComSource.h"

NS_HJ_BEGIN

HJRenderFaceuObject::HJRenderFaceuObject()
{
	HJ_SetInsName(HJRenderFaceuObject);
}
HJRenderFaceuObject::~HJRenderFaceuObject()
{
    HJFLogi("~HJRenderFaceuObject");
	m_faceu = nullptr;
}
int HJRenderFaceuObject::init(HJBaseParam::Ptr i_param)
{
    HJFLogi("HJRenderFaceuObject::init() enter");
    int i_err = HJ_OK;
    do
    {
        m_faceu = HJRteComSourceFaceu::Create();
        if (!m_faceu)
        {
            i_err = HJErrNewObj;
            HJFLoge("HJRenderFaceuObject::init() HJRteComSourceFaceu::Create() failed.");
            break;
        }
        i_err = m_faceu->init(i_param);       
        if (i_err != HJ_OK)
        {
            HJFLoge("HJRenderFaceuObject::init() m_faceu->init() failed.");
            break;
        }
	} while (0);
    HJFLogi("HJRenderFaceuObject::init() called. i_err:{}", i_err);
	return i_err;
}

int HJRenderFaceuObject::render(int i_width, int i_height)
{
	int i_err = HJ_OK;
	do
	{
		if (!m_faceu)
		{
            i_err = HJErrInvalidParams;
			break;
		}
		m_faceu->adjustResolution(i_width, i_height);
		i_err = m_faceu->update(nullptr);
        if (i_err < 0)
        {
            HJFLoge("HJRenderFaceuObject::render() m_faceu->update() failed. i_err:{}", i_err);
            break;
        }
	} while (0);
	return i_err;
}



NS_HJ_END
