#include "HJRenderContextExport.h"
#include "HJPrerequisites.h"
#include "HJEntryContext.h"
#include "HJRenderCoreSingleton.h"
#include "HJError.h"
#include "HJFLog.h"

NS_HJ_USING

int renderContextInit(const HJEntryContextInfo& i_contextInfo)
{
	int i_err = HJ_OK;
	do
	{
		i_err = HJEntryContext::init(HJEntryType_Render, i_contextInfo);
		if (i_err < 0)
		{
			break;
		}
		i_err = HJRenderCoreSingleton::Instance().init();
        if (i_err < 0)
        {
            break;
        }
	} while (false);
	return i_err;
}

void renderContextUnInit()
{
	HJRenderCoreSingleton::Instance().done();
}

