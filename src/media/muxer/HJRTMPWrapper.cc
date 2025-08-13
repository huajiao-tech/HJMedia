//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRTMPWrapper.h"
#include "HJFLog.h"
#include "librtmp/rtmp.h"
#include "librtmp/log.h"
#include <cstdint>

//#define HJ_HAVE_RTMPEX	1
#if defined(HJ_HAVE_RTMPEX)
#	include "librtmp/net-if.h"
#endif // 

#if defined(HJ_OS_WIN32)
#include <winsock2.h>
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#endif

#if defined( __cplusplus )
extern "C" {
#endif
#include "libavformat/network.h"
#if defined( __cplusplus )
}
#endif

//#if defined(ANDROID_LIB) || defined(IOS_LIB) || defined(HarmonyOS)
//#include <sys/ioctl.h>
//#endif

#define HJRTMP_MAX_STREAMS 8
#define HJRTMP_HEAD_SIZE   (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

#define HJRTMP_LOG_LEVEL   RTMP_LOGWARNING

NS_HJ_BEGIN
//***********************************************************************************//
HJRTMPWrapper::HJRTMPWrapper(HJRTMPWrapperDelegate::Wtr delegate)
	: m_delegate(delegate)
{
	RTMP_LogSetCallback(HJRTMPWrapper::rtmp_log);
	RTMP_LogSetLevel(HJRTMP_LOG_LEVEL);
}

HJRTMPWrapper::~HJRTMPWrapper()
{
	done();
}

int HJRTMPWrapper::init(const std::string url, HJOptions::Ptr opts)
{
	int res = HJ_OK;
	do
	{
		//HJ_AUTOU_LOCK(m_mutex);
		int64_t t0 = HJCurrentSteadyMS();
		m_url = url;
		size_t last_slash_pos = m_url.find_last_of('/');
		if (last_slash_pos != std::string::npos) {
			m_key = m_url.substr(last_slash_pos + 1);
		}
		if (opts) {
			if (opts->haveValue(HJRTMPUtils::STORE_KEY_SOCKET_BUF_SIZE)) {
				m_socketBufferSize = opts->getInt(HJRTMPUtils::STORE_KEY_SOCKET_BUF_SIZE);
			}
		}
		HJFLogi("entry, m_url:{}, m_socketBufferSize:{}, version:{}", m_url, m_socketBufferSize, HJ_VERSION);

		m_rtmp = RTMP_Alloc();
		if (!m_rtmp) {
			HJFLoge("rtmp alloc failed");
			return HJErrNewObj;
		}
		RTMP_Init(m_rtmp);

//        if (AF_UNSPEC != m_ipToConnect.ss_family) {
//            #ifdef __APPLE__
//                (void)memcpy(&_rtmp->Link.ipToConnect, &m_ipToConnect, m_ipToConnect.ss_len);
//            #else
//                (void)memcpy(&m_rtmp->Link.ipToConnect, &m_ipToConnect, sizeof(m_rtmp->Link.ipToConnect));
//            #endif
//        }
		//
		RTMP* rtmp = m_rtmp;
		if (!RTMP_SetupURL(rtmp, (char*)m_url.c_str())) {
			set_output_error();
			HJLoge("error, rtmp setup url:" + m_url);
			res = HJErrInvalidUrl;
			break;
		}
		RTMP_EnableWrite(rtmp);

#if defined(HJ_HAVE_RTMPEX)
		set_rtmp_dstr(&rtmp->Link.pubUser, m_username);
		set_rtmp_dstr(&rtmp->Link.pubPasswd, m_password);
		set_rtmp_dstr(&rtmp->Link.flashVer, m_encoderName);
		rtmp->Link.swfUrl = rtmp->Link.tcUrl;
		//
		if (m_bindIP.empty() || "default" == m_bindIP) {
			memset(&rtmp->m_bindIP, 0, sizeof(rtmp->m_bindIP));
		}
		else {
			bool success = netif_str_to_addr(&rtmp->m_bindIP.addr,
				&rtmp->m_bindIP.addrLen,
				m_bindIP.c_str());
			if (success) {
				int len = rtmp->m_bindIP.addrLen;
				bool ipv6 = len == sizeof(struct sockaddr_in6);
				HJFLogi("Binding to IPv{}", ipv6 ? 6 : 4);
			}
		}
		// Only use the IPv4 / IPv6 hint if a binding address isn't specified.
		if (rtmp->m_bindIP.addrLen == 0)
			rtmp->m_bindIP.addrLen = m_addrLenHint;

		RTMP_AddStream(rtmp, m_key.c_str());

		rtmp->m_outChunkSize = 4096;
		rtmp->m_bSendChunkSizeInfo = true;
		rtmp->m_bUseNagle = true;
#endif
		if (m_socketBufferSize > 0) {
			rtmp->socket_buffer_size = m_socketBufferSize;
		}
       rtmp->Link.timeout = 10;  //s

		HJLogi("rtmp connect before");
		if (!RTMP_Connect(rtmp, NULL)) {
			set_output_error();
			HJLogi("error, rtmp connect");
			res = HJErrNetConnectFailed;
			break;
		}
		HJLogi("rtmp connect ok");
		notify(std::move(HJMakeNotification(HJRTMP_EVENT_CONNECTED, "rtmp connected")));

		if (!RTMP_ConnectStream(rtmp, 0)) {
			set_output_error();
			HJLoge("error, connect stream");
			res = HJErrNetInvalidStream;
			break;
		}
		m_connected = true;

#if defined(HJ_HAVE_RTMPEX)
		char ip_address[INET6_ADDRSTRLEN] = { 0 };
		netif_addr_to_str(&rtmp->m_sb.sb_addr, ip_address, INET6_ADDRSTRLEN);
#else
		if (!set_chunk_size()) {
			HJLoge("error, rtmp set_chunk_size fail");
			return false;
		}
#endif
		HJLogi("rtmp connect stream ok");

		logSndbufSize();
		HJFLogi("Connection to {} success!, time:{}", m_url, HJCurrentSteadyMS() - t0);
	} while (false);

	HJLogi("end");
	if (HJ_OK != res) 
	{
		done();
		switch (res) {
		case HJErrNetConnectFailed:
			notify(std::move(HJMakeNotification(HJRTMP_EVENT_CONNECT_FAILED, HJFMT("rtmp connect error:{}", m_lastError))));
			break;
		case HJErrNetInvalidStream:
			notify(std::move(HJMakeNotification(HJRTMP_EVENT_STREAM_CONNECT_FAIL, HJFMT("rtmp stream connect error:{}", m_lastError))));
			break;
		default:
			break;
		}
	} else {
		notify(std::move(HJMakeNotification(HJRTMP_EVENT_STREAM_CONNECTED, "rtmp stream connected")));
	}

	return res;
}

int HJRTMPWrapper::send(const uint8_t* data, size_t len, int idx)
{
	if (!m_rtmp || (idx > HJRTMP_MAX_STREAMS)) {
		HJFLoge("invalid rtmp or stream index:{}", idx);
		return HJErrNotAlready;
	}
#if defined(HJ_HAVE_RTMPEX)
	int wlen = RTMP_Write(m_rtmp, (const char*)data, (int)len, idx);
#else
	int wlen = RTMP_Write(m_rtmp, (const char*)data, (int)len);
#endif
	if (wlen <= 0) {
		set_output_error();
		HJFLoge("RTMP_Write failed, len:{}, wlen:{}, last_error:{}", len, wlen, m_lastError);
		notify(std::move(HJMakeNotification(HJRTMP_EVENT_SEND_Error, HJFMT("rtmp send error:{}", m_lastError))));
		m_lastCode = HJErrNetSend;
		return m_lastCode;
	}
	//HJFLogi("rtmp write tag data size:{}, out size:{}", len, wlen);
	return HJ_OK;
}

int HJRTMPWrapper::recv()
{
	int ret = 0;
	int recv_size = 0;
	if (!m_newSocketLoop) {
#ifdef _WIN32
		ret = ioctlsocket(m_rtmp->m_sb.sb_socket, FIONREAD,
			(u_long*)&recv_size);
#else
		ret = ioctl(m_rtmp->m_sb.sb_socket, FIONREAD, &recv_size);
#endif

		if (ret >= 0 && recv_size > 0) {
			if (!processRecvData((size_t)recv_size)) {
				notify(std::move(HJMakeNotification(HJRTMP_EVENT_RECV_Error, HJFMT("rtmp recv error:{}", m_lastError))));
				m_lastCode = HJErrNetRecv;
				return m_lastCode;
			}
		}
	}
	return HJ_OK;
}

void HJRTMPWrapper::setQuit(const bool isQuit)
{
    HJLogi("entry");
	//HJ_AUTOU_LOCK(m_mutex);
	if (m_rtmp && (INVALID_SOCKET != m_rtmp->m_sb.sb_socket)) {
		HJLogi("entry, close");
		RTMPSockBuf_Close(&m_rtmp->m_sb);
		m_rtmp->m_sb.sb_socket = INVALID_SOCKET;
		HJLogi("end, close");
	}
    HJLogi("end");
}

void HJRTMPWrapper::close()
{
	if (!m_rtmp) {
		return;
	}
	HJFLogi("entry, m_connected:{}", m_connected);
	if(m_connected) {
		RTMP_Close(m_rtmp);
		m_connected = false;
	}
	HJLogi("end");
	return;
}

void HJRTMPWrapper::done()
{
	if (!m_rtmp) {
		return;
	}
	auto t0 = HJCurrentSteadyMS();
	HJFLogi("entry, m_connected:{}", m_connected);

	set_output_error();
	if(m_connected) {
		RTMP_Close(m_rtmp);
		m_connected = false;
	}

#if defined(HJ_HAVE_RTMPEX)
	RTMP_TLS_Free(m_rtmp);
#endif

	RTMP_Free(m_rtmp);
	m_rtmp = NULL;
	notify(std::move(HJMakeNotification(HJRTMP_EVENT_DISCONNECTED, "rtmp disconnected")));

	HJFLogi("end, time:{}", HJCurrentSteadyMS() - t0);
}

int HJRTMPWrapper::setNONBlocking()
{
	if (!m_rtmp) {
		return HJErrNotAlready;
	}
	int one = 1;
#ifdef _WIN32
	if (ioctlsocket(m_rtmp->m_sb.sb_socket, FIONBIO, (u_long*)&one)) {
		//m_rtmp->last_error_code = WSAGetLastError();
#else
	if (ioctl(m_rtmp->m_sb.sb_socket, FIONBIO, &one)) {
		//m_rtmp->last_error_code = errno;
#endif
		HJLoge("Failed to set non-blocking socket");
		return HJErrNetNonBlocking;
	}
	return HJ_OK;
}

bool HJRTMPWrapper::processRecvData(size_t size)
{
	//UNUSED_PARAMETER(size);

	RTMP* rtmp = m_rtmp;
	RTMPPacket packet = { 0 };

	if (!RTMP_ReadPacket(rtmp, &packet)) {
#ifdef _WIN32
		int error = WSAGetLastError();
#else
		int error = errno;
#endif
		HJFLogi("RTMP_ReadPacket error: %d", error);
		return false;
	}

	if (packet.m_body) {
		/* do processing here */
		RTMPPacket_Free(&packet);
	}
	return true;
}

void HJRTMPWrapper::rtmp_log(int level, const char* format, va_list args)
{
	if (level > HJRTMP_LOG_LEVEL)
		return;

	char* strp = NULL;
	int res = vasprintf(&strp, format, args);
	if (res > 0 && strp) {
		HJFLogi("librtmp log:{}", strp);
		free(strp);
	}
	return;
}

void HJRTMPWrapper::set_rtmp_dstr(AVal* val, std::string& str)
{
	if (!str.empty()) {
		val->av_val = (char*)str.c_str();
		val->av_len = (int)str.length();
	}
	else {
		val->av_val = NULL;
		val->av_len = 0;
	}
}

void HJRTMPWrapper::logSndbufSize()
{
	SOCKET fd = m_rtmp->m_sb.sb_socket;
	int rcv_buf_size = 0;
	socklen_t rcv_buf_len = sizeof(rcv_buf_size);
	if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&rcv_buf_size, &rcv_buf_len)) {
		HJFLoge("error, get SO_RCVBUF failed:{}", ff_neterrno());
	}
	int snd_buf_size = 0;
	socklen_t snd_buf_len = sizeof(snd_buf_size);
	if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&snd_buf_size, &snd_buf_len)) {
		HJFLoge("error, get SO_RCVBUF failed:{}", ff_neterrno());
	}
	timeval rcv_timeout{ 0,0 }, snd_timeout{ 0,0 };
	socklen_t opt_len = sizeof(timeval);
	if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcv_timeout, &opt_len) < 0) {
		HJFLoge("error, get SO_RCVTIMEO failed:{}", ff_neterrno());
	}
	if (getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&snd_timeout, &opt_len) < 0) {
		HJFLoge("error, get SO_RCVTIMEO failed:{}", ff_neterrno());
	}
	HJFLogi("end, SO_RCVBUF:{}, SO_SNDBUF:{}, SO_RCVTIMEO:{}, SO_SNDTIMEO:{}", rcv_buf_size, snd_buf_size, (rcv_timeout.tv_sec * 1000000 + rcv_timeout.tv_usec) / 1000, (snd_timeout.tv_sec * 1000000 + snd_timeout.tv_usec) / 1000);
	return;
}


std::string HJRTMPWrapper::set_output_error()
{
#if defined(HJ_HAVE_RTMPEX)
	int last_error_code = m_rtmp->last_error_code;
#else
	int last_error_code = m_rtmp->m_errno;
#endif
	//
	std::string msg = "";
#ifdef _WIN32
	switch (last_error_code) {
	case WSAETIMEDOUT:
		msg = "ConnectionTimedOut";
		break;
	case WSAEACCES:
		msg = "PermissionDenied";
		break;
	case WSAECONNABORTED:
		msg = "ConnectionAborted";
		break;
	case WSAECONNRESET:
		msg = "ConnectionReset";
		break;
	case WSAHOST_NOT_FOUND:
		msg = "HostNotFound";
		break;
	case WSANO_DATA:
		msg = "NoData";
		break;
	case WSAEADDRNOTAVAIL:
		msg = "AddressNotAvailable";
		break;
	case WSAEINVAL:
		msg = "InvalidParameter";
		break;
	case WSAEHOSTUNREACH:
		msg = "NoRoute";
		break;
	}
#else
	switch (last_error_code) {
	case ETIMEDOUT:
		msg = "ConnectionTimedOut";
		break;
	case EACCES:
		msg = "PermissionDenied";
		break;
	case ECONNABORTED:
		msg = "ConnectionAborted";
		break;
	case ECONNRESET:
		msg = "ConnectionReset";
		break;
	case HOST_NOT_FOUND:
		msg = "HostNotFound";
		break;
	case NO_DATA:
		msg = "NoData";
		break;
	case EADDRNOTAVAIL:
		msg = "AddressNotAvailable";
		break;
	case EINVAL:
		msg = "InvalidParameter";
		break;
	case EHOSTUNREACH:
		msg = "NoRoute";
		break;
	}
#endif

	// non platform-specific errors
	if (msg.empty()) {
		switch (last_error_code) {
		case -0x2700:
			msg = "SSLCertVerifyFailed";
			break;
		case -0x7680:
			msg = "Failed to load root certificates for a secure TLS connection."
#if defined(__linux__)
				" Check you have an up to date root certificate bundle in /etc/ssl/certs."
#endif
				;
			break;
		default:
			msg = HJFMT("unkown error code:{}", last_error_code);
			break;
		}
	}

	if (!msg.empty()) {
		m_lastError = msg;
	}
	return msg;
}

bool HJRTMPWrapper::set_chunk_size()
{
	HJFLogi("entry");
	RTMPPacket pack;
	RTMPPacket_Alloc(&pack, 4);
	pack.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
	pack.m_nChannel = 0x02;
	pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
	pack.m_nTimeStamp = 0;
	pack.m_nInfoField2 = 0;
	pack.m_nBodySize = 4;

	int nVal = 1024;
	pack.m_body[3] = nVal & 0xff;
	pack.m_body[2] = nVal >> 8;
	pack.m_body[1] = nVal >> 16;
	pack.m_body[0] = nVal >> 24;
	m_rtmp->m_outChunkSize = nVal;
	bool ret = (0 != RTMP_SendPacket(m_rtmp, &pack, FALSE));
	RTMPPacket_Free(&pack);
	return ret;
}

bool HJRTMPWrapper::sendFooter(int timestamp, bool isHevc)
{
	HJFLogi("entry, isHevc:{}", isHevc);
	RTMPPacket* packet;
	unsigned char* body;

	char data[6] = { 0, 0, 0, 1, 0x0B,1 };
	uint32_t data_size = 1, tmp_size;
	int length = 5;

	
	if (isHevc) {
		data[4] = 0x4A;// NALU type 37, reserved
		data[5] = 0x01;
		length = 6;
		data_size = 2;
	}

	packet = (RTMPPacket*)malloc(HJRTMP_HEAD_SIZE + length + 5);
	memset(packet, 0, HJRTMP_HEAD_SIZE);

	packet->m_body = (char*)packet + HJRTMP_HEAD_SIZE;
	body = (unsigned char*)packet->m_body;

	if (isHevc) {
		body[0] = 0x2c;
	}
	else {
		body[0] = 0x27;
	}

	body[1] = 0x01;
	body[2] = 0;
	body[3] = 0;
	body[4] = 0;
	//
	tmp_size = htonl(data_size);
	memcpy(body + 5, &tmp_size, 4);
	memcpy(body + 5 + 4, &data[4], data_size);

	packet->m_nBodySize = 5 + length;
	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nChannel = RTMP_PACKET_TYPE_CONTROL + 1;
	packet->m_nTimeStamp = timestamp + 10;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nInfoField2 = m_rtmp->m_stream_id;
	RTMP_SendPacket(m_rtmp, packet, FALSE);
	free(packet);
	HJFLogi("end");
	return true;
}


NS_HJ_END
