#include "HJInferenceContextExport.h"
#include "HJPrerequisites.h"
#include "HJEntryContext.h"
#include "HJError.h"
#include "HJFLog.h"

NS_HJ_USING

int inferenceContextInit(const HJEntryContextInfo& i_contextInfo)
{
	int i_err = HJ_OK;
	do
	{
		i_err = HJEntryContext::init(HJEntryType_Inference, i_contextInfo);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}

void inferenceContextUnInit()
{
}

