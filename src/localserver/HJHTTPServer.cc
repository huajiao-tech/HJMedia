//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJHTTPServer.h"
#include "HJFLog.h"
#include "HJMediaUtils.h"
#include "HJFileUtil.h"
#include "HJException.h"
#include <filesystem>

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

namespace fs = std::filesystem;

NS_HJ_BEGIN
//***********************************************************************************//
HJHTTPServer::HJHTTPServer(const HJServerParams& params, HJServerDelegate::Wtr delegate)
    : m_params(params)
    , m_delegate(delegate)
{
    m_server = std::make_unique<httplib::Server>();
    m_executor = HJCreateExecutor(HJMakeGlobalName("HJHTTPServer"));
}

HJHTTPServer::~HJHTTPServer() 
{
    stop();
}

int HJHTTPServer::start() 
{
    HJFLogi("entry");
    m_status = HJSTATUS_Inited;
    m_executor->async([&]() {
        setupServer();
    });
    return HJ_OK;
}

void HJHTTPServer::stop() 
{
    HJLogi("entry");
    {
        HJ_AUTOU_LOCK(m_mutex);
        m_status = HJSTATUS_Done;
    }
    if (m_server) {
        m_server->stop();
        m_server = nullptr;
    }
    notify(std::move(HJMakeNotification(HJ_SERVER_NOTIFY_STOP, "http server stop")));
    //
    if (m_executor) {
        m_executor->stop();
        m_executor = nullptr;
    }
    HJLogi("end");

    return;
}

bool HJHTTPServer::isRunning()
{
    HJ_AUTOU_LOCK(m_mutex);
    return (HJSTATUS_Running == m_status);
}

void HJHTTPServer::setupRoutes()
{
    m_server->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        handleMediaList(req, res);
    });

    m_server->Get(R"(/play)", [this](const httplib::Request& req, httplib::Response& res) {
        handlePlayMedias(req, res);
    });
    //m_server->Get(R"(/videos/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
    //    handle_video_stream(req, res);
    //    });
    return;
}

int HJHTTPServer::setupServer()
{
    HJFLogi("entry");
    {
        HJ_AUTOU_LOCK(m_mutex);
        if (HJSTATUS_Done == m_status) {
            return HJErrNotAlready;
        }
        setupRoutes();
        //
        if (!m_server->bind_to_port(m_params.ip, m_params.port)) {
            HJFLogi("http server bind failed");
            notify(std::move(HJMakeNotification(HJ_SERVER_NOTIFY_ERROR, "http server start failed")));
            return HJErrFatal;
        }
        HJFLogi("http server start ok");
        m_status = HJSTATUS_Running;
        notify(std::move(HJMakeNotification(HJ_SERVER_NOTIFY_START, "http server start ok")));
    }
    //
    if (!m_server->listen_after_bind()) {
        HJFLogi("http server listen failed");
        notify(std::move(HJMakeNotification(HJ_SERVER_NOTIFY_ERROR, "http server listen failed")));
        return HJErrFatal;
    }

    return HJ_OK;
}

void HJHTTPServer::handleMediaList(const httplib::Request& req, httplib::Response& res) 
{
    std::stringstream html;
    html << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Video List</title>
    <style>
        body { font-family: sans-serif; margin: 2em; background-color: #f4f4f4; }
        ul { list-style-type: none; padding: 0; }
        li { margin: 0.5em 0; }
        a { text-decoration: none; color: #0366d6; font-size: 1.2em; }
        a:hover { text-decoration: underline; }
        .container { max-width: 800px; margin: auto; background: white; padding: 2em; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
    </style>
</head>
<body>
    <div class="container">
        <h1>Available Videos</h1>
        <ul>)";

    auto files = HJMediaUtils::enumMediaFiles(m_params.media_dir);
    for (const auto& filename : files) {
        std::string mime_type = get_mime_type(filename);
        if (mime_type.rfind("video/", 0) == 0) {
            html << "<li><a href=\"/videos/" << filename << "\">" << filename << "</a></li>";
        }
    }

    html << R"(        </ul>
    </div>
</body>
</html>)";

    res.set_content(html.str(), "text/html");
}

void HJHTTPServer::handlePlayMedias(const httplib::Request& req, httplib::Response& res)
{
    HJFLogi("http request entry");
    for (const auto& header : req.headers) {
        HJFLogi("request headers:{} - {}", header.first, header.second);
    }
    //
	auto reqInfo = getRequest(req);
    if (reqInfo.url.empty()) {
        res.status = 400; // Bad Request
        res.set_content("Missing file parameter", "text/plain");
        return;
    }
    HJFLogi("http request req info url:{}, rid:{}, level:{}", reqInfo.url, reqInfo.rid, reqInfo.level);

    if (HJURL_MODE_LOCAL == reqInfo.urlMode && !HJFileUtil::fileExist(reqInfo.url.c_str())) {
        res.status = 404;
        return;
    }
    std::string mime_type = get_mime_type(reqInfo.url);
    res.set_header("Accept-Ranges", "bytes");
    res.set_header("Cache-Control", "public, max-age=3600"); // Cache for 1 hour
    if (!reqInfo.rid.empty()) {
        res.set_header("ETag", reqInfo.rid);
    }
    res.set_header("X-Cache", "HIT"); // HIT, MISS
    res.set_header("X-Cache-Hits", 0);

#if 0
    auto file_size = fs::file_size(file_path);

    if (req.has_header("Range")) {
        std::string range_header = req.get_header_value("Range");
        int64_t start = 0, end = -1;
        
        std::string range_spec = range_header.substr(range_header.find('=') + 1);
        size_t dash_pos = range_spec.find('-');
        if (dash_pos != std::string::npos) {
            try {
                start = std::stoll(range_spec.substr(0, dash_pos));
                std::string end_str = range_spec.substr(dash_pos + 1);
                if (!end_str.empty()) {
                    end = std::stoll(end_str);
                } else {
                    end = file_size - 1;
                }
            } catch (const std::exception&) {
                res.status = 400; // Bad Request
                return;
            }
        }

        if (start >= file_size) {
            res.status = 416; // Range Not Satisfiable
            res.set_header("Content-Range", "bytes */" + std::to_string(file_size));
            return;
        }

        if (end < 0 || end >= file_size) {
            end = file_size - 1;
        }

        auto content_length = end - start + 1;
        res.status = 206; // Partial Content
        res.set_header("Content-Range", "bytes " + std::to_string(start) + "-" + std::to_string(end) + "/" + std::to_string(file_size));
        HJFLogi("http request range: {}-{}, content_length:{}, file_size:{}", start, end, content_length, file_size);

        res.set_content_provider(
            static_cast<size_t>(file_size),
            mime_type,
            [file_path, start](size_t offset, size_t length, httplib::DataSink& sink) {
                HJFLogi("http request range: {}-{}, length:{}", start, start + offset, length);
                std::ifstream file(file_path, std::ios::binary);
                if (!file) return false;
                file.seekg(start, std::ios::beg);
                std::vector<char> buf(length);
                file.read(buf.data(), length);
                sink.write(buf.data(), file.gcount());
                return true;
            }
        );
    } else {
        res.status = 200; // OK

        res.set_content_provider(
            static_cast<size_t>(file_size),
            mime_type,
            [file_path](size_t offset, size_t length, httplib::DataSink& sink) {
                std::ifstream file(file_path, std::ios::binary);
                if (!file) return false;
                file.seekg(offset, std::ios::beg);
                std::vector<char> buf(length);
                file.read(buf.data(), length);
                sink.write(buf.data(), file.gcount());
                return true;
            }
        );
    }

#endif

    return;
}

std::string HJHTTPServer::get_mime_type(const std::string& path) 
{
    std::string ext = fs::path(path).extension().string();
    std::map<std::string, std::string> mime_types = {
        {".mp4", "video/mp4"},
        {".mov", "video/quicktime"},
        {".avi", "video/x-msvideo"},
        {".mkv", "video/x-matroska"},
        {".webm", "video/webm"},
        {".flv", "video/x-flv"},
        {".wmv", "video/x-ms-wmv"},
        {".ogv", "video/ogg"}
    };

    if (mime_types.count(ext)) {
        return mime_types[ext];
    }
    return "application/octet-stream";
}

/**
* http://127.0.0.1:8125/play?id=3981106772274880398&u=E:/movies/blob/server/2024_08_19_11_01_48_371_783.flv&level=0
* http://127.0.0.1:8125/videos/3981106772274880398.mp4
*/
HJServerRequest HJHTTPServer::getRequest(const httplib::Request& req)
{
    HJServerRequest request;
    if (req.has_param("id")) {
        request.rid = req.get_param_value("id");
    }
    if (req.has_param("u")) {
        request.url = req.get_param_value("u");
        request.urlMode = HJURL_MODE_REMOTE;
    }
    if (req.has_param("level")) {
        request.level = std::stoi(req.get_param_value("level"));
    }
    ///videos/*
    if (req.matches.size() > 1) 
    {
        request.url = req.matches[1];
        if (!m_params.media_dir.empty()) {
            request.url = HJUtilitys::concatenatePath(m_params.media_dir, request.url);
        }
        request.urlMode = HJURL_MODE_LOCAL;
    }
    // Security: prevent path traversal attacks
    if (request.url.find("..") != std::string::npos) {
        HJFLogw("Path traversal attack detected: {}", request.url);
        request.url = "";
    }
    return request;
}

//***********************************************************************************//
NS_HJ_END