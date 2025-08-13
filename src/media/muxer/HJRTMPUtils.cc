//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRTMPUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJEnumToStringFuncImplBegin(HJRTMPEEvent)
	HJEnumToStringItem(HJRTMP_EVENT_NONE),
	HJEnumToStringItem(HJRTMP_EVENT_CONNECTED),
	HJEnumToStringItem(HJRTMP_EVENT_CONNECT_FAILED),
	HJEnumToStringItem(HJRTMP_EVENT_STREAM_CONNECTED),
	HJEnumToStringItem(HJRTMP_EVENT_STREAM_CONNECT_FAIL),
	HJEnumToStringItem(HJRTMP_EVENT_DISCONNECTED),
	HJEnumToStringItem(HJRTMP_EVENT_DROP_FRAME),
	HJEnumToStringItem(HJRTMP_EVENT_AUTOADJUST_BITRATE),
	HJEnumToStringItem(HJRTMP_EVENT_SEND_Error),
	HJEnumToStringItem(HJRTMP_EVENT_RECV_Error),
	HJEnumToStringItem(HJRTMP_EVENT_RETRY),
	HJEnumToStringItem(HJRTMP_EVENT_RETRY_TIMEOUT),
	HJEnumToStringItem(HJRTMP_EVENT_DONE),
HJEnumToStringFuncImplEnd(HJRTMPEEvent);

//***********************************************************************************//
const std::string HJRTMPUtils::STORE_KEY_LOCALURL = "local_url";
const std::string HJRTMPUtils::STORE_KEY_SOCKET_BUF_SIZE = "socket_buf_size";
const std::string HJRTMPUtils::STORE_KEY_RETRY_TIME_LIMITED = "retry_time_limited";
const std::string HJRTMPUtils::STORE_KEY_DROP_ENABLE = "drop_enale";
const std::string HJRTMPUtils::STORE_KEY_DROP_THRESHOLD = "drop_threshold";
const std::string HJRTMPUtils::STORE_KEY_DROP_PFRAME_THRESHOLD = "drop_pframe_threshold";
const std::string HJRTMPUtils::STORE_KEY_DROP_IFRAME_THRESHOLD = "drop_iframe_threshold";
//
const std::string HJRTMPUtils::STORE_KEY_BITRATE = "bitrate";
const std::string HJRTMPUtils::STORE_KEY_LOW_BITRATE = "low_bitrate";
const std::string HJRTMPUtils::STORE_KEY_ADJUST_INTERVAL = "adjust_interval";

NS_HJ_END