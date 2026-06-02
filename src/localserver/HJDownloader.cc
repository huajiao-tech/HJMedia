//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJDownloader.h"
#include "HJFLog.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"
#include <fstream>
#include <memory>
#include <regex>

NS_HJ_BEGIN
// Default chunk size for downloading: 1MB
const int64_t CHUNK_SIZE = 1024 * 1024;
//***********************************************************************************//
HJDownloader::HJDownloader(std::string url, std::string localUrl)
    : m_url(std::move(url)), m_localUrl(std::move(localUrl))
{
}

HJDownloader::~HJDownloader() = default;

bool HJDownloader::parseUrl()
{
    std::regex url_regex(R"(^(http|https)://([^/]+)(/.*)?$)");
    std::smatch match;
    if (std::regex_match(m_url, match, url_regex))
    {
        std::string protocol = match[1].str();
        m_host = match[2].str();
        m_path = match[3].str().empty() ? "/" : match[3].str();

        if (protocol == "https")
        {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            m_client = std::make_unique<httplib::Client>(m_host);
            m_client->enable_server_certificate_verification(false); // For simplicity, disable verification. In production, you should handle certificates properly.
#else
            m_last_error = "HTTPS support is not enabled in cpp-httplib.";
            return false;
#endif
        }
        else
        {
            m_client = std::make_unique<httplib::Client>(m_host);
        }
        return true;
    }
    else
    {
        m_last_error = "Invalid URL format: " + m_url;
        HJFLoge(m_last_error);
        return false;
    }
}

bool HJDownloader::fetchContentLength()
{
    auto res = m_client->Head(m_path.c_str());
    if (!res)
    {
        m_last_error = "Failed to connect to host or send HEAD request.";
        return false;
    }

    // --- Start: Print Response Headers ---
    HJFLogd("--- Server Response Headers ---");
    for (const auto& header : res->headers) {
        HJFLogd("{}: {}", header.first, header.second);
    }
    HJFLogd("-----------------------------");
    // --- End: Print Response Headers ---

    if (res->status != 200)
    {
        m_last_error = "Server returned status " + std::to_string(res->status) + " for HEAD request: " + m_path;
        HJFLoge(m_last_error);
        return false;
    }
    //for (const auto& header : res->headers) {
    //    HJFLogi("response headers:{} - {}", header.first, header.second);
    //}

    if (res->has_header("Content-Length"))
    {
        m_total_length = std::stoll(res->get_header_value("Content-Length"));
        return true;
    }
    else
    {
        m_last_error = "Server did not provide Content-Length header.";
        return false;
    }
}

void HJDownloader::setOnProgress(const ProgressCallback& callback)
{
    m_progress_callback = callback;
}

std::string HJDownloader::getLastError() const
{
    return m_last_error;
}

bool HJDownloader::start()
{
    if (m_is_running) {
        m_last_error = "Downloader is already running.";
        return false;
    }

    m_is_running = true;
    bool success = [this]() {
        int64_t t0 = HJCurrentSteadyMS();
        if (!parseUrl())
        {
            return false;
        }

        if (!fetchContentLength())
        {
            return false;
        }

        std::ofstream file(m_localUrl, std::ios::binary | std::ios::app);
        if (!file.is_open())
        {
            m_last_error = "Failed to open file for writing: " + m_localUrl;
            HJFLoge(m_last_error);
            return false;
        }

        m_downloaded_length = file.tellp();

        if (m_downloaded_length >= m_total_length)
        {
            if (m_progress_callback)
            {
                m_progress_callback(m_total_length, m_total_length);
            }
            HJFLogi("File already downloaded: {}", m_localUrl);
            return true;
        }

        while (m_downloaded_length < m_total_length)
        {
            int64_t start_byte = m_downloaded_length;
            int64_t end_byte = std::min(start_byte + CHUNK_SIZE - 1, m_total_length - 1);

            httplib::Headers headers = {
                {"Range", "bytes=" + std::to_string(start_byte) + "-" + std::to_string(end_byte)}
            };

            auto res = m_client->Get(m_path.c_str(), headers, 
                [&](const char *data, size_t data_length) {
                    file.write(data, data_length);
                    if (!file)
                    {
                        m_last_error = "File write error: " + m_localUrl;
                        HJFLoge(m_last_error);
                        return false;
                    }

                    m_downloaded_length += data_length;
                    if (m_progress_callback)
                    {
                        m_progress_callback(m_downloaded_length, m_total_length);
                    }
                    return true; // Continue receiving data
                });

            if (!res)
            {
                if (m_last_error.empty()) {
                    m_last_error = "Download failed: connection error or content receiver returned false.";
                }
                HJFLoge(m_last_error);
                return false;
            }

            if (res->status != 206 && res->status != 200)
            {
                m_last_error = "Download failed: Server returned status " + std::to_string(res->status);
                HJFLoge(m_last_error);
                return false;
            }
        }

        file.flush();
        file.close();
        HJFLogi("Download finished. path: {}, time: {} ms", m_localUrl, HJCurrentSteadyMS() - t0);
        return true;
    }();

    m_is_running = false;
    return success;
}

//***********************************************************************************//
NS_HJ_END
