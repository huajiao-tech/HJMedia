//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJServerUtils.h"
#include "HJExecutor.h"

// Forward declarations
namespace httplib {
    class Server;
    struct Request;
    struct Response;
}

NS_HJ_BEGIN
//***********************************************************************************//
//typedef struct HJHTTPServerParams
//{
//    std::string ip = "0.0.0.0";
//    int port = HJ_HTTP_PORT_DEFAULT;
//    std::string media_dir = "";
//};

typedef enum HJUrlMode
{
    HJURL_MODE_NONE = 0,
    HJURL_MODE_REMOTE = 1,
    HJURL_MODE_LOCAL = 2,
} HJUrlMode;

typedef struct HJServerRequest
{
    std::string rid = "";
    int level = 0;
    std::string url = "";
    HJUrlMode urlMode = HJURL_MODE_NONE;
} HJServerRequest;

class HJHTTPServer : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJHTTPServer);

    HJHTTPServer(const HJServerParams& params, HJServerDelegate::Wtr delegate);
    virtual ~HJHTTPServer();

    // Starts the server in a background thread. Returns true on success.
    int start();

    // Stops the server.
    void stop();

    // Checks if the server is currently running.
    bool isRunning();
private:
    // Sets up the HTTP routes (handlers).
    void setupRoutes();

    int setupServer();

    // Handler to generate and serve the video file list as an HTML page.
    void handleMediaList(const httplib::Request& req, httplib::Response& res);

    // Handler to stream a specific video file, supporting Range requests.
    void handlePlayMedias(const httplib::Request& req, httplib::Response& res);

    // Helper to get the MIME type from a file extension.
    std::string get_mime_type(const std::string& path);

    HJServerRequest getRequest(const httplib::Request& req);
    //
    virtual int notify(HJNotification::Ptr ntf) {
        auto delegate = HJLockWtr<HJServerDelegate>(m_delegate);
        if (delegate) {
            return delegate->onLocalServerNotify(std::move(ntf));
        }
        return HJ_OK;
    }
private:
    std::unique_ptr<httplib::Server> m_server;
    HJServerParams          m_params;
    HJServerDelegate::Wtr   m_delegate;
    HJExecutor::Ptr         m_executor{};
    std::mutex              m_mutex;
    HJStatus                m_status{ HJSTATUS_NONE };
};

NS_HJ_END
