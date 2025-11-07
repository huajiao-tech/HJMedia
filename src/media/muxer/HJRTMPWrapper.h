//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJRTMPUtils.h"
#include "HJNotify.h"
#include "HJException.h"

typedef struct RTMP RTMP;
typedef struct AVal AVal;

NS_HJ_BEGIN
//***********************************************************************************//
class HJRTMPWrapperDelegate : public virtual HJObject
{
public:
    HJ_DECLARE_PUWTR(HJRTMPWrapperDelegate)
    //
    virtual int onRTMPWrapperNotify(HJNotification::Ptr ntf) = 0;
    virtual HJBuffer::Ptr onAcquireMediaTag(int64_t timeout, bool isHeader = false) = 0;
};

class HJRTMPWrapper : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJRTMPWrapper>;
    HJRTMPWrapper(HJRTMPWrapperDelegate::Wtr delegate);
    virtual ~HJRTMPWrapper();

    virtual int init(const std::string url, HJOptions::Ptr opts = nullptr);
    virtual int send(const uint8_t* data, size_t len, int idx);
    virtual int recv();
    virtual void setQuit(const bool isQuit = true);
    virtual void close();
    virtual void done();
    //
    virtual std::string getLastError() {
        return m_lastError;
    }
    bool sendFooter(int timestamp, bool isHevc);
private:
    int setNONBlocking();
    //int get_sndbuf_remaining();
    bool processRecvData(size_t size);
    //
    virtual int notify(HJNotification::Ptr ntf) {
        auto delegate = HJLockWtr<HJRTMPWrapperDelegate>(m_delegate);
        if (delegate) {
            return delegate->onRTMPWrapperNotify(std::move(ntf));
        }
        return HJ_OK;
    }

    static void rtmp_log(int level, const char* format, va_list args);
    //
    void set_rtmp_dstr(AVal* val, std::string& str);
    void logSndbufSize();
    std::string set_output_error();
    bool set_chunk_size();
private:
    HJRTMPWrapperDelegate::Wtr m_delegate;
    std::string         m_url{ "rtmp://localhost/live" };         //path
    std::string		    m_key{ "livestream" };
    std::string		    m_username = "";
    std::string		    m_password = "";
    std::string		    m_encoderName{ "FMLE/3.0 (compatible; FMSc/1.0)" };
    std::string		    m_bindIP = "default";
    std::string         m_IPFamily = "IPv4+IPv6";               //"IPv4"  IPv6
    int		            m_addrLenHint = 0;	                    /* hint IPv4 vs IPv6 */ //addrlen_hint
    int                 m_socketBufferSize = 0;

	//std::mutex          m_mutex;
    bool                m_newSocketLoop = false;        //new_socket_loop
    bool                m_lowLatencyMode = false;       //low_latency_mode
    //
    RTMP*               m_rtmp{ NULL };
    bool                m_connected{ false };
	int 			    m_lastCode{ HJ_OK };
    std::string		    m_lastError{ "" };
};

NS_HJ_END