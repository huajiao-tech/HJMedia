#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "HJUtilitys.h"
#include "HJExecutor.h"

typedef struct URLContext URLContext;
typedef struct AVDictionary AVDictionary;
typedef struct TLSShared TLSShared;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct bio_method_st BIO_METHOD;
typedef struct bio_st BIO;

NS_HJ_BEGIN
/////////////////////////////////////////////////////////////////////////////
class HJTLSOpenSSLLink : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJTLSOpenSSLLink>;
	using WPtr = std::weak_ptr<HJTLSOpenSSLLink>;
	HJTLSOpenSSLLink(const std::string& name, HJOptions::Ptr opts = nullptr);
	virtual ~HJTLSOpenSSLLink();

	int tls_open(const std::string& uri, int flags, AVDictionary** options = NULL);
	int tls_reopen(AVDictionary** options = NULL);
	int tls_read(uint8_t* buf, int size);
	int tls_write(const uint8_t* buf, int size);
	int tls_get_file_handle();
	int tls_get_short_seek();
	int tls_close();

	virtual void setQuit(const bool isQuit = true) {
		m_isQuit = isQuit;
	}
	virtual bool getIsQuit() {
		return m_isQuit;
	}
	//void setUsed(bool used) {
	//	m_used = used;
	//}
	//bool getUsed() {
	//	return m_used;
	//}
	const int64_t getStartTime() {
		return m_startTime;
	}
	const int getFlags() {
		return flags;
	}
	const std::string& getUri() {
		return m_uri;
	}
private:
	static int onInterruptCB(void* ctx);
	int print_tls_error(int ret);
	//
	static int url_bio_create(BIO* b);
	static int url_bio_destroy(BIO* b);
	static int url_bio_bread(BIO* b, char* buf, int len);
	static int url_bio_bwrite(BIO* b, const char* buf, int len);
	static long url_bio_ctrl(BIO* b, int cmd, long num, void* ptr);
	static int url_bio_bputs(BIO* b, const char* str);
private:
	std::string			m_uri{ "" };
	int					flags;
	URLContext*			m_uc{ NULL };
	std::atomic<bool>   m_isQuit = { false };
	//std::atomic<bool>   m_used{ true };
	//
	TLSShared*			tls_shared{ NULL };
	SSL_CTX*			ctx{ NULL };
	SSL*				ssl{ NULL };
	BIO_METHOD*			url_bio_method{ NULL };
	int					io_err = 0;

	int64_t				m_startTime{0};
    int                 m_openCnt{0};
	std::string			m_objName{ "" };
};
using HJTLSOpenSSLLinkList = std::list<HJTLSOpenSSLLink::Ptr>;

/////////////////////////////////////////////////////////////////////////////
class HJTLSLinkManager : public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJTLSLinkManager);
	HJTLSLinkManager();
	virtual ~HJTLSLinkManager();

	HJ_INSTANCE_DECL(HJTLSLinkManager);

	int init();
	void done();
	void terminate();

	void setDefaultUrl(const std::string& url);

	size_t getTLSLink(const std::string url, int flags, AVDictionary** options = NULL);

	HJTLSOpenSSLLink::Ptr createTLSLink(const std::string url, int flags, AVDictionary** options = NULL);
	void addTLSLink(HJTLSOpenSSLLink::Ptr& link);
	//int unusedTLSLink(size_t handle);
//	int freeTLSLink(size_t handle);

	void asyncInitTLSLink(const std::string& url);
	void reInitTLSLink(size_t handle);
	void asyncDelayTLSTask(size_t handle, int64_t delayTime);

	HJTLSOpenSSLLink::Ptr getUsedLinkByID(size_t handle);
	//
	HJKeyStorage::Ptr getProcInfoCategory(const std::string& category);
	void eraseProcInfoCategory(const std::string& category);
	std::string getProcInfoCategoryString(const std::string& category);
	void setProcInfo(const std::string& category, const std::string& key, const std::any& value);
	//options
	void setLinkMax(int max) {
        m_urlLinkMax = max;
	}
	void setLinkAliveTime(int64_t time) {
		m_urlLinkAliveTime = time;
	}
private:
	HJTLSOpenSSLLink::Ptr getrLinkByName(const std::string& name);
	HJTLSOpenSSLLink::Ptr getrLinkByID(size_t handle);

	int getLinkCountByName(const std::string& name);
	std::string getValidLinkUrl();
	static const size_t getGlobalID();
	//
	void procLinkUrl();
	std::string getTLSUrl(const std::string& url);

	AVDictionary* getDefaultOptions();
private:
    std::atomic<bool>                                           m_security = { false };
	std::unordered_multimap<std::string, HJTLSOpenSSLLinkList>	m_nameLinks;
//	std::unordered_map<size_t, HJTLSOpenSSLLink::Ptr>           m_usedLinks;
	std::recursive_mutex										m_mutex;
	//pools
	int															m_defaultFlags{3};
	std::list<std::string>										m_defaultUrls;
	std::atomic<int>											m_urlLinkMax{ 4 };
	std::atomic<int64_t>										m_urlLinkAliveTime{ 15 };		//s
	HJExecutor::Ptr												m_executor{ nullptr };
	//HJTask::Ptr												m_timer{ nullptr };
	//uint64_t													m_defaultInterval{ 5 };
	//
	std::recursive_mutex										m_procMutex;
	std::unordered_map<std::string, HJKeyStorage::Ptr>			m_procInfos;
};
/////////////////////////////////////////////////////////////////////////////
NS_HJ_END

#define HJTLSLinkManagerInstance()  HJ::HJTLSLinkManager::getInstance()
#define HJTLSLinkManagerDestroy()	HJ::HJTLSLinkManager::destoryInstance()
#define HJTMGetProcCategoryString(category)		HJTLSLinkManagerInstance()->getProcInfoCategoryString(category)
#define HJTMEraseProcCategory(category)			HJTLSLinkManagerInstance()->eraseProcInfoCategory(category)
#define HJTMSetProcInfo(category, key, value)	HJTLSLinkManagerInstance()->setProcInfo(category, key, value)

#define HJTLSLinkManagerInit()                  HJTLSLinkManagerInstance()->init()
#define HJTLSLinkManagerDone()                  HJTLSLinkManagerInstance()->done()
#define HJTLSLinkManagerSetDefaultUrl(url)		HJTLSLinkManagerInstance()->setDefaultUrl(url)
