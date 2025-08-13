//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNotify.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJEnumToStringFuncImplBegin(HJNotifyID)
	HJEnumToStringItem(HJNotify_NONE),
	HJEnumToStringItem(HJNotify_Prepared),
	HJEnumToStringItem(HJNotify_Already),
	HJEnumToStringItem(HJNotify_NeedWindow),
	HJEnumToStringItem(HJNotify_WindowChanged),
	HJEnumToStringItem(HJNotify_PlaySuccess),
	HJEnumToStringItem(HJNotify_DemuxEnd),
	HJEnumToStringItem(HJNotify_Complete),
	HJEnumToStringItem(HJNotify_LoadingStart),
	HJEnumToStringItem(HJNotify_LoadingEnd),
	HJEnumToStringItem(HJNotify_ProgressStatus),
	HJEnumToStringItem(HJNotify_Error),
HJEnumToStringFuncImplEnd(HJNotifyID);

NS_HJ_END

