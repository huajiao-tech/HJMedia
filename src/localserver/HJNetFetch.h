//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaUtils.h"
#include "HJNotify.h"
#include <atomic>
#include <mutex>

namespace httplib {
	class Client;
	struct Request;
	struct Response;
}

NS_HJ_BEGIN
//***********************************************************************************//
class HJNetFetchDelegate : public virtual HJObject
{
public:
    HJ_DECLARE_PUWTR(HJNetFetchDelegate);

    virtual int onNetFetchNotify(HJNotification::Ptr ntfy) = 0;
};

typedef enum HJNetFetchStatus {
	HJNETFETCH_INIT = 0,
	HJNETFETCH_FETCHING,
	HJNETFETCH_DONE,
	HJNETFETCH_ERROR,
} HJNetFetchStatus;

class HJNetFetch : public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJNetFetch);

	HJNetFetch(size_t block_size, const HJNetFetchDelegate::Wtr& delegate);
	virtual ~HJNetFetch() noexcept;

	int init(const std::string& url, bool verify_ssl = true);
	void done();

	int fetch(HJRange64i range);

	int64_t getLength() const {
		return m_total_length;
	}

	std::string getLastError() const {
		return m_last_error;
	}
private:
	bool parseUrl(bool verify_ssl);
	int64_t getContentLength();

	void printHeaders(const httplib::Response* res);

	virtual int notify(HJNotification::Ptr ntf) {
        auto delegate = HJLockWtr<HJNetFetchDelegate>(m_delegate);
        if (delegate) {
            return delegate->onNetFetchNotify(std::move(ntf));
        }
        return HJ_OK;
    }
private:
	HJNetFetchDelegate::Wtr   m_delegate;
	std::string m_url{""};
	std::string m_host{""};
    std::string m_path{""};
	std::unique_ptr<httplib::Client> m_client;
	std::string m_last_error;
	std::mutex m_mutex;
	int64_t m_block_size{0};
	int64_t m_total_length = 0;
	std::atomic<bool> m_quit{false};
};

NS_HJ_END