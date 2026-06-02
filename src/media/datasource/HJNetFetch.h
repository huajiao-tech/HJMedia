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
#include <memory>
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
	HJNETFETCH_START = 0,
	HJNETFETCH_FETCHING,
	HJNETFETCH_END,
	HJNETFETCH_ERROR,
} HJNetFetchStatus;

class HJNetFetch : public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJNetFetch);

	static constexpr size_t kFetchBlockSizeDefault = 1024 * 1024;

	HJNetFetch(size_t block_size = kFetchBlockSizeDefault, const HJNetFetchDelegate::Ptr& delegate = nullptr);
	virtual ~HJNetFetch() noexcept;

	int init(const std::string& url, bool verify_ssl = true);
	void done();

	int fetch(HJRange64i range, HJListener callback = nullptr);

	int64_t getLength() const {
		return m_total_length.load();
	}

	std::string getLastError() const {
        std::lock_guard<std::mutex> lock(m_mutex);
		return m_last_error;
	}
	void setTimeout(int64_t timeout) {
		m_rwTimeout = timeout;
	}
	int64_t getTimeout() const {
		return m_rwTimeout;
	}
	void setBlockSize(size_t block_size) {
		m_block_size = block_size;
	}
	size_t getBlockSize() const {
		return m_block_size;
	}
private:
	bool parseUrl(bool verify_ssl);
	int64_t getContentLength();

	void printHeaders(const httplib::Response* res);
    void setLastError(const std::string& error);

	virtual int notify(HJNotification::Ptr ntf) {
        auto delegate = HJLockWtr<HJNetFetchDelegate>(m_delegate);
        if (delegate) {
            return delegate->onNetFetchNotify(std::move(ntf));
        }
        return HJ_OK;
    }
private:
	HJNetFetchDelegate::Wtr	m_delegate;
	std::string 			m_url{""};
	std::string 			m_host{""};
    std::string 			m_path{""};
	std::shared_ptr<httplib::Client> m_client;
	std::string 			m_last_error;
	mutable std::mutex 		m_mutex;
	int64_t 				m_block_size{kFetchBlockSizeDefault};
	std::atomic<int64_t> 	m_total_length{0};
    std::atomic<bool> 		m_fetching{false};
	std::atomic<bool> 		m_quit{false};
	int64_t 				m_rwTimeout{3000};	//ms
};

NS_HJ_END
