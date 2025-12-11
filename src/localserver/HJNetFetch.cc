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
HJNetFetch::HJNetFetch(size_t block_size, const HJNetFetchDelegate::Wtr& delegate)
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
    m_url = url;
    auto isOK = parseUrl(verify_ssl);
    if (!isOK) {
        return HJErrInvalidUrl;
    }
    m_total_length = getContentLength();
    if(m_total_length <= 0) {
        return HJErrInvalid;
    }

    return HJ_OK;
}
void HJNetFetch::done()
{
    m_quit = true;
    HJ_AUTO_LOCK(m_mutex);
    if(m_client) {
        m_client->stop();
        m_client = nullptr;
    }
}

int HJNetFetch::fetch(HJRange64i range)
{
    HJ_AUTO_LOCK(m_mutex);
    if(!m_client) {
        return HJErrNotAlready;
    }
    int64_t downloaded_length = 0;
    int64_t fetch_length = range.end - range.begin + 1;
    if (fetch_length > m_total_length) {
        fetch_length = m_total_length;
    }
    HJFLogi("Fetching range {}-{}/{}", range.begin, range.end, fetch_length);

    if(!m_delegate.expired()) {
        auto ntfy = HJMakeNotification(HJNETFETCH_INIT, HJFMT("range:{}-{}/{}", range.begin, range.end, fetch_length));
        notify(std::move(ntfy));
    }
    
    int ret = HJ_OK;
    while (!m_quit && (downloaded_length < fetch_length))
    {
        int64_t start_byte = range.begin + downloaded_length;
        int64_t end_byte = std::min(start_byte + m_block_size - 1, range.end); 
        //
        httplib::Headers headers = {
            {"Range", "bytes=" + std::to_string(start_byte) + "-" + std::to_string(end_byte)}
        };

        auto res = m_client->Get(m_path.c_str(), headers, 
            [&](const char *data, size_t data_length) {
                auto buffer = HJCreates<HJBuffer>((uint8_t *)data, data_length);
                downloaded_length += data_length;

                if (!m_delegate.expired()) {
                    auto end_pos = start_byte + data_length - 1;
                    auto ntfy = HJMakeNotification(HJNETFETCH_FETCHING, HJFMT("range:{}-{}/{}", start_byte, end_pos, data_length));
                    (*ntfy)["data"] = buffer;
                    HJRange64i range{start_byte, end_byte};
                    (*ntfy)["range"] = range;
                    notify(std::move(ntfy));
                }
                return true;
            }
        );

        if (!res) {
            m_last_error = "Download failed: connection error or content receiver returned false.";
            ret = HJErrDownloadFailed;
            break;
        }

        if (res->status != 206 && res->status != 200) {
            m_last_error = "Download failed: Server returned status " + std::to_string(res->status);
            ret = HJErrDownloadFailed;
            break;
        }
    }
    
    if (!m_delegate.expired()) {
        auto ntfy = HJMakeNotification(HJNETFETCH_DONE, ret, HJFMT("range:{}-{}/{}", range.begin, range.end, downloaded_length));
        notify(std::move(ntfy));
    }
    
    HJFLogi("Download completed. Total bytes downloaded: {}, ret:{}", downloaded_length, ret);

    return ret;
}

bool HJNetFetch::parseUrl(bool verify_ssl)
{
    static const std::regex url_regex(R"(^(http|https)://([^/]+)(/.*)?$)");
    std::smatch match;
    if (std::regex_match(m_url, match, url_regex))
    {
        std::string protocol = match[1].str();
        m_host = match[2].str();
        m_path = match[3].str().empty() ? "/" : match[3].str();

        if (protocol == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            m_client = std::make_unique<httplib::Client>(m_host);
            m_client->enable_server_certificate_verification(verify_ssl);
#else
            m_last_error = "HTTPS support is not enabled in cpp-httplib.";
            return false;
#endif
        } else {
            m_client = std::make_unique<httplib::Client>(m_host);
        }
        return true;
    }
    m_last_error = "Invalid URL format.";
    return false;
}

int64_t HJNetFetch::getContentLength()
{
    auto res = m_client->Head(m_path);
    if (!res) {
        m_last_error = "Failed to connect to host or send HEAD request.";
        return 0;
    }
    printHeaders(&res.value());

    if (res->status != 200) {
        m_last_error = "Server returned status " + std::to_string(res->status) + " for HEAD request.";
        return 0;
    }
    if (res->has_header("Content-Length")) {
        return std::stoll(res->get_header_value("Content-Length"));
    }
    m_last_error = "Server did not provide Content-Length header.";
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


NS_HJ_END