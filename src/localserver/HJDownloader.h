//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include <string>
#include <functional>
#include <cstdint>

// Forward declaration to avoid including httplib.h in a public header
namespace httplib {
    class Client;
}

NS_HJ_BEGIN
//***********************************************************************************//
class HJDownloader : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJDownloader);

    // Callback function for progress updates
    // Parameters: (current downloaded bytes, total file bytes)
    using ProgressCallback = std::function<void(int64_t, int64_t)>;

    HJDownloader(std::string url, std::string localUrl);
    virtual ~HJDownloader();

    // Starts the download. Returns true on success, false on failure.
    bool start();

    // Sets the progress callback function.
    void setOnProgress(const ProgressCallback& callback);

    // Gets the last error message if start() returned false.
    std::string getLastError() const;

private:
    // Parses the URL to extract host and path.
    bool parseUrl();

    // Gets the total content length of the remote file.
    bool fetchContentLength();

private:
    std::string m_url;
    std::string m_localUrl;
    std::string m_host;
    std::string m_path;
    
    std::unique_ptr<httplib::Client> m_client;

    int64_t m_total_length = 0;
    int64_t m_downloaded_length = 0;

    ProgressCallback m_progress_callback;
    std::string m_last_error;
};

NS_HJ_END
