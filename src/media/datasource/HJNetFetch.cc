//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJNetFetch.h"
#include "HJFLog.h"
#include "HJException.h"
#include "HJFileUtil.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"
NS_HJ_BEGIN
//***********************************************************************************//
HJNetFetch::HJNetFetch(size_t block_size, const HJNetFetchDelegate::Ptr& delegate)
    : m_block_size(block_size)
    , m_delegate(delegate)
{

}

HJNetFetch::~HJNetFetch()
{
    done();
}

int HJNetFetch::init(const std::string& url, bool verify_ssl)
{
    if (m_fetching.load()) {
        setLastError("fetch already running.");
        return HJErrAlreadyExist;
    }
    if (m_block_size <= 0) {
        setLastError("invalid block size.");
        return HJErrInvalidParams;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_url = url;
        m_host.clear();
        m_path.clear();
        m_client.reset();
        m_last_error.clear();
    }
    m_quit = false;
    auto is_ok = parseUrl(verify_ssl);
    if (!is_ok) {
        return HJErrInvalidUrl;
    }
    int64_t total_length = getContentLength();
    m_total_length.store(total_length);
    if (total_length <= 0) {
        return HJErrInvalid;
    }

    return HJ_OK;
}
void HJNetFetch::done()
{
    m_quit = true;
    std::shared_ptr<httplib::Client> client;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        client = m_client;
        m_client.reset();
    }
    if (client) {
        client->stop();
    }
}

int HJNetFetch::fetch(HJRange64i range, HJListener callback)
{
    if (m_block_size <= 0) {
        setLastError("invalid block size.");
        return HJErrInvalidParams;
    }
    auto t0 = HJCurrentSteadyMS();
    bool expected = false;
    if (!m_fetching.compare_exchange_strong(expected, true)) {
        setLastError("fetch already running.");
        return HJErrAlreadyExist;
    }
    struct FetchGuard {
        std::atomic<bool>& flag;
        ~FetchGuard() { flag.store(false); }
    } guard{m_fetching};

    if (m_quit.load()) {
        setLastError("fetch canceled.");
        return HJErrNetCanceled;
    }

    std::shared_ptr<httplib::Client> client;
    std::string path;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        client = m_client;
        path = m_path;
    }
    if (!client) {
        setLastError("client not initialized.");
        return HJErrNotAlready;
    }
    int64_t total_length = m_total_length.load();
    if (total_length <= 0) {
        setLastError("invalid content length.");
        return HJErrInvalid;
    }
    if (range.begin < 0 || range.end < 0 || range.end < range.begin) {
        setLastError("invalid range.");
        return HJErrInvalidParams;
    }
    if (range.begin >= total_length) {
        setLastError("range begin out of bounds.");
        return HJErrInvalidParams;
    }

    int64_t downloaded_length = 0;
    int64_t range_end = std::min(range.end, total_length - 1);
    if (range_end < range.begin) {
        setLastError("invalid range end.");
        return HJErrInvalidParams;
    }
    int64_t fetch_length = range_end - range.begin + 1;
    if (fetch_length <= 0) {
        setLastError("invalid fetch length.");
        return HJErrInvalidParams;
    }
    HJFLogi("entry, range {}-{}/{}", range.begin, range_end, fetch_length);

    auto startNtfy = HJMakeNotification(HJNETFETCH_START, HJFMT("range:{}-{}/{}", range.begin, range_end, fetch_length));
    if(!m_delegate.expired()) {
        notify(startNtfy);
    }
    if(callback) {
        callback(startNtfy);
    }
    
    int ret = HJ_OK;
    bool require_partial = (range.begin > 0 || range_end < (total_length - 1));
    while (!m_quit && (downloaded_length < fetch_length))
    {
        int64_t start_byte = range.begin + downloaded_length;
        int64_t end_byte = std::min(start_byte + m_block_size - 1, range_end); 
        if (end_byte < start_byte) {
            setLastError("invalid block range.");
            ret = HJErrInvalidParams;
            break;
        }
        //
        httplib::Headers headers = {
            {"Range", "bytes=" + std::to_string(start_byte) + "-" + std::to_string(end_byte)}
        };

        auto res = client->Get(path.c_str(), headers,
            [&](const httplib::Response& response) {
                if (m_quit.load()) {
                    return false;
                }
                if (require_partial && response.status != 206) {
                    setLastError("server did not honor range request.");
                    return false;
                }
                if (!require_partial && response.status != 206 && response.status != 200) {
                    setLastError("Download failed: Server returned status " + std::to_string(response.status));
                    return false;
                }
                return true;
            },
            [&](const char *data, size_t data_length) {
                if (m_quit.load()) {
                    return false;
                }
                if (data_length == 0) {
                    return true;
                }
                auto buffer = HJCreates<HJBuffer>((uint8_t *)data, data_length);
                downloaded_length += static_cast<int64_t>(data_length);
                //
                int64_t current_chunk_start = range.begin + downloaded_length - static_cast<int64_t>(data_length);
                int64_t current_chunk_end = range.begin + downloaded_length - 1;
                auto ntfy = HJMakeNotification(HJNETFETCH_FETCHING, HJFMT("range:{}-{}/{}", current_chunk_start, current_chunk_end, data_length));
                (*ntfy)["data"] = buffer;
                HJRange64i data_range{current_chunk_start, current_chunk_end};
                (*ntfy)["range"] = data_range;
                if (!m_delegate.expired()) {
                    notify(ntfy);
                }
                if(callback) {
                    int r = callback(ntfy);
                    if (r != HJ_OK) {
                        setLastError("fetch canceled by callback.");
                        return false;
                    }
                }
                return true;
            }
        );

        if (!res) {
            if (m_quit.load()) {
                setLastError("fetch canceled.");
                ret = HJErrNetCanceled;
            } else {
                if (getLastError().empty()) {
                    setLastError("Download failed: connection error or content receiver returned false");
                }
                ret = HJErrDownloadFailed;
            }
            break;
        }

        if (res->status != 206 && res->status != 200) {
            setLastError(HJFMT("Download failed: Server returned status:{}, reason:{}", res->status, res->reason));
            ret = HJErrDownloadFailed;
            break;
        }
    }
    
    auto endNtfy = HJMakeNotification(HJNETFETCH_END, ret, HJFMT("range:{}-{}/{}", range.begin, range_end, downloaded_length));
    if (!m_delegate.expired()) {
        notify(endNtfy);
    }
    if(callback) {
        callback(endNtfy);
    }
    
    HJFLogi("end, completed Total bytes: {}, ret:{}, time:{}ms", downloaded_length, ret, HJCurrentSteadyMS() - t0);

    return ret;
}

bool HJNetFetch::parseUrl(bool verify_ssl)
{
    static const std::regex url_regex(R"(^(http|https)://([^/]+)(/.*)?$)");
    std::smatch match;
    std::string url;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        url = m_url;
    }
    if (std::regex_match(url, match, url_regex))
    {
        std::string protocol = match[1].str();
        std::string host = match[2].str();
        std::string path = match[3].str().empty() ? "/" : match[3].str();
        std::shared_ptr<httplib::Client> client;

        if (protocol == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            client = std::make_shared<httplib::Client>(host);
            client->enable_server_certificate_verification(verify_ssl);
#else
            setLastError("HTTPS support is not enabled in cpp-httplib.");
            return false;
#endif
        } else {
            client = std::make_shared<httplib::Client>(host);
        }
        if (m_rwTimeout > 0) {
            time_t sec = static_cast<time_t>(m_rwTimeout / 1000);
            time_t usec = static_cast<time_t>((m_rwTimeout % 1000) * 1000);
            client->set_connection_timeout(sec, usec);
            client->set_read_timeout(sec, usec);
            client->set_write_timeout(sec, usec);
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_host = host;
            m_path = path;
            m_client = std::move(client);
        }
        return true;
    }
    setLastError("Invalid URL format.");
    return false;
}

int64_t HJNetFetch::getContentLength()
{
    std::shared_ptr<httplib::Client> client;
    std::string path;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        client = m_client;
        path = m_path;
    }
    if (!client) {
        setLastError("client not initialized.");
        return 0;
    }
    auto res = client->Head(path);
    if (!res) {
        setLastError("Failed to connect to host or send HEAD request.");
        return 0;
    }
    printHeaders(&res.value());

    if (res->status != 200) {
        setLastError("Server returned status " + std::to_string(res->status) + " for HEAD request.");
        return 0;
    }
    if (res->has_header("Content-Length")) {
        return std::stoll(res->get_header_value("Content-Length"));
    }
    setLastError("Server did not provide Content-Length header.");
    return 0;
}

void HJNetFetch::printHeaders(const httplib::Response* res)
{
    HJFLogi("--- Server Response Headers ---");
    for (const auto& header : res->headers) {
        HJFLogi("{} - {}", header.first, header.second);
    }
    HJFLogi("-----------------------------");
}

void HJNetFetch::setLastError(const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_last_error = error;
}


NS_HJ_END
