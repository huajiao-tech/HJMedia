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
#include <iostream>
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
        m_last_error = "Invalid URL format.";
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
    std::cout << "--- Server Response Headers ---" << std::endl;
    for (const auto& header : res->headers) {
        std::cout << header.first << ": " << header.second << std::endl;
    }
    std::cout << "-----------------------------" << std::endl;
    // --- End: Print Response Headers ---

    if (res->status != 200)
    {
        m_last_error = "Server returned status " + std::to_string(res->status) + " for HEAD request.";
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
        return false;
    }

    m_downloaded_length = file.tellp();

    if (m_downloaded_length >= m_total_length)
    {
        if (m_progress_callback)
        {
            m_progress_callback(m_total_length, m_total_length);
        }
        std::cout << "File already downloaded." << std::endl;
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
                    // This error occurs inside a lambda, so we have to handle it carefully.
                    // Returning false will stop the httplib request processing.
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
            m_last_error = "Download failed: connection error or content receiver returned false.";
            return false;
        }

        if (res->status != 206 && res->status != 200)
        {
            m_last_error = "Download failed: Server returned status " + std::to_string(res->status);
            return false;
        }
    }

    file.close();
    std::cout << "Download finished." << std::endl;
    return true;
}

//***********************************************************************************//
NS_HJ_END
