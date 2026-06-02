//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJLiveOnLineView.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>

#include "HJFLog.h"

NS_HJ_BEGIN

namespace {

constexpr float kCardWidth = 168.0f;
constexpr float kCardHeight = 308.0f;
constexpr float kThumbWidth = 144.0f;
constexpr float kThumbHeight = 180.0f;
constexpr float kCardSpacing = 10.0f;
constexpr float kCardColumnsPreferred = 6.0f;
const char* kDefaultCredentialPath = "E:/lzy/hj_key.txt";
const char* kDefaultLivePageUrl = "https://admin.huajiao.com/Resource/LiveResource/index";
const char* kThumbOutputRootDir = "E:/thumb";
const char* kThumbLevelItems[] = { "good", "middle", "bad" };
constexpr int kThumbCaptureTargetCount = 5;
constexpr int64_t kThumbCaptureIntervalMs = 2000;

const char* kLoginTypes[] = { "qihoo", "system" };

struct LiveCredentialConfig {
    std::string m_user;
    std::string m_password;
    std::vector<std::string> m_live_urls;
};

std::string chooseStreamUrlFor(bool i_prefer_flv, const HJLiveResourceItem& i_item)
{
#if HJ_XMEDIATOOLS_HAS_LIBCURL
    return HJLiveResourceFetcher::choosePlayableUrl(i_item, i_prefer_flv);
#else
    if (i_prefer_flv) {
        if (!i_item.m_flvurl.empty()) {
            return i_item.m_flvurl;
        }
        return i_item.m_m3u8;
    }
    if (!i_item.m_m3u8.empty()) {
        return i_item.m_m3u8;
    }
    return i_item.m_flvurl;
#endif
}

void copyStringToBuffer(const std::string& i_value, char* o_buffer, size_t i_size)
{
    if (o_buffer == nullptr || i_size == 0) {
        return;
    }
    std::memset(o_buffer, 0, i_size);
    std::snprintf(o_buffer, i_size, "%s", i_value.c_str());
}

std::string trimString(const std::string& i_value)
{
    size_t begin = 0;
    while (begin < i_value.size() &&
           std::isspace(static_cast<unsigned char>(i_value[begin])) != 0) {
        ++begin;
    }

    size_t end = i_value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(i_value[end - 1])) != 0) {
        --end;
    }
    return i_value.substr(begin, end - begin);
}

std::string toLowerString(std::string i_value)
{
    std::transform(i_value.begin(), i_value.end(), i_value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return i_value;
}

uint64_t makeStableHash(const std::string& i_value)
{
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < i_value.size(); ++i) {
        hash ^= static_cast<unsigned char>(i_value[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string hashToHex(uint64_t i_value)
{
    std::ostringstream stream;
    stream << std::hex << i_value;
    return stream.str();
}

std::string guessImageExtension(const std::string& i_url)
{
    std::string value = i_url;
    const size_t query_pos = value.find('?');
    if (query_pos != std::string::npos) {
        value = value.substr(0, query_pos);
    }

    const size_t dot_pos = value.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return ".jpg";
    }

    std::string ext = toLowerString(value.substr(dot_pos));
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
        return ext;
    }
    return ".jpg";
}

bool readTextFile(const std::string& i_path, std::string* o_content, std::string* o_error)
{
    if (o_content == nullptr) {
        return false;
    }

    std::ifstream stream(i_path.c_str(), std::ios::in | std::ios::binary);
    if (!stream.is_open()) {
        if (o_error != nullptr) {
            *o_error = HJFMT("open {} failed", i_path);
        }
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    if (!stream.good() && !stream.eof()) {
        if (o_error != nullptr) {
            *o_error = HJFMT("read {} failed", i_path);
        }
        return false;
    }

    *o_content = buffer.str();
    return true;
}

bool extractJsonStringField(const std::string& i_content,
                            const std::string& i_key,
                            std::string* o_value)
{
    if (o_value == nullptr) {
        return false;
    }

    const std::regex pattern(
        HJFMT("\"{}\"\\s*:\\s*\"([^\"\\\\]*(?:\\\\.[^\"\\\\]*)*)\"", i_key));
    std::smatch match;
    if (!std::regex_search(i_content, match, pattern) || match.size() < 2) {
        return false;
    }

    std::string value = match[1].str();
    std::string decoded;
    decoded.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            const char escaped = value[i + 1];
            switch (escaped) {
                case '\\':
                case '"':
                case '/':
                    decoded.push_back(escaped);
                    ++i;
                    continue;
                case 'b':
                    decoded.push_back('\b');
                    ++i;
                    continue;
                case 'f':
                    decoded.push_back('\f');
                    ++i;
                    continue;
                case 'n':
                    decoded.push_back('\n');
                    ++i;
                    continue;
                case 'r':
                    decoded.push_back('\r');
                    ++i;
                    continue;
                case 't':
                    decoded.push_back('\t');
                    ++i;
                    continue;
                default:
                    break;
            }
        }
        decoded.push_back(value[i]);
    }

    *o_value = decoded;
    return true;
}

std::vector<std::string> extractJsonStringValues(const std::string& i_content)
{
    std::vector<std::string> values;
    const std::regex pattern("\"([^\"\\\\]*(?:\\\\.[^\"\\\\]*)*)\"");
    auto begin = std::sregex_iterator(i_content.begin(), i_content.end(), pattern);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        std::string value;
        if (extractJsonStringField(HJFMT("{{\"value\":\"{}\"}}", (*it)[1].str()), "value", &value)) {
            value = trimString(value);
            if (!value.empty()) {
                values.push_back(value);
            }
        }
    }
    return values;
}

bool extractJsonStringArrayField(const std::string& i_content,
                                 const std::string& i_key,
                                 std::vector<std::string>* o_values)
{
    if (o_values == nullptr) {
        return false;
    }

    const std::regex pattern(HJFMT("\"{}\"\\s*:\\s*\\[([\\s\\S]*?)\\]", i_key));
    std::smatch match;
    if (!std::regex_search(i_content, match, pattern) || match.size() < 2) {
        return false;
    }

    *o_values = extractJsonStringValues(match[1].str());
    return !o_values->empty();
}

bool loadDefaultCredentials(LiveCredentialConfig* o_config, std::string* o_error)
{
    if (o_config == nullptr) {
        return false;
    }

    std::string content;
    if (!readTextFile(kDefaultCredentialPath, &content, o_error)) {
        return false;
    }

    LiveCredentialConfig config;
    extractJsonStringField(content, "user", &config.m_user);
    extractJsonStringField(content, "password", &config.m_password);
    if (!extractJsonStringArrayField(content, "live_url", &config.m_live_urls)) {
        std::string live_url;
        extractJsonStringField(content, "live_url", &live_url);
        live_url = trimString(live_url);
        if (!live_url.empty()) {
            config.m_live_urls.push_back(live_url);
        }
    }
    config.m_user = trimString(config.m_user);
    config.m_password = trimString(config.m_password);

    if (config.m_user.empty() && config.m_password.empty() && config.m_live_urls.empty()) {
        if (o_error != nullptr) {
            *o_error = HJFMT("{} missing user/password/live_url", kDefaultCredentialPath);
        }
        return false;
    }

    if ((!config.m_user.empty() && config.m_password.empty()) ||
        (config.m_user.empty() && !config.m_password.empty())) {
        if (o_error != nullptr) {
            *o_error = HJFMT("{} user/password should be set together", kDefaultCredentialPath);
        }
        return false;
    }

    *o_config = config;
    return true;
}

std::filesystem::path ensureThumbCacheDir()
{
    const std::filesystem::path cache_dir =
        std::filesystem::path(HJUtilitys::exeDir()) / "cache" / "live_online" / "thumbs";
    std::error_code error;
    std::filesystem::create_directories(cache_dir, error);
    return cache_dir;
}

std::string makeThumbCachePath(const std::string& i_url)
{
    const std::filesystem::path cache_dir = ensureThumbCacheDir();
    const std::string filename = hashToHex(makeStableHash(i_url)) + guessImageExtension(i_url);
    return (cache_dir / filename).u8string();
}

size_t writeBinaryCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    const size_t real_size = size * nmemb;
    auto* data = static_cast<std::vector<uint8_t>*>(userdata);
    if (data == nullptr || ptr == nullptr || real_size == 0) {
        return 0;
    }
    data->insert(data->end(), ptr, ptr + real_size);
    return real_size;
}

bool downloadThumbnailFile(const std::string& i_url,
                           const std::string& i_output_path,
                           std::string* o_error)
{
#if !HJ_XMEDIATOOLS_HAS_LIBCURL
    HJ_UNUSED(i_url);
    HJ_UNUSED(i_output_path);
    if (o_error != nullptr) {
        *o_error = "libcurl disabled";
    }
    return false;
#else
    const CURLcode init_result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (init_result != CURLE_OK) {
        if (o_error != nullptr) {
            *o_error = "curl_global_init failed";
        }
        return false;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        curl_global_cleanup();
        if (o_error != nullptr) {
            *o_error = "curl_easy_init failed";
        }
        return false;
    }

    std::vector<uint8_t> image_bytes;
    curl_easy_setopt(curl, CURLOPT_URL, i_url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBinaryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &image_bytes);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                     "AppleWebKit/537.36 (KHTML, like Gecko) "
                     "Chrome/145.0.0.0 Safari/537.36");

    const CURLcode request_result = curl_easy_perform(curl);
    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (request_result != CURLE_OK) {
        if (o_error != nullptr) {
            *o_error = curl_easy_strerror(request_result);
        }
        return false;
    }
    if (status_code != 200L) {
        if (o_error != nullptr) {
            *o_error = HJFMT("http status {}", status_code);
        }
        return false;
    }
    if (image_bytes.empty()) {
        if (o_error != nullptr) {
            *o_error = "empty image payload";
        }
        return false;
    }

    std::ofstream stream(i_output_path.c_str(), std::ios::binary);
    if (!stream.is_open()) {
        if (o_error != nullptr) {
            *o_error = "open cache file failed";
        }
        return false;
    }
    stream.write(reinterpret_cast<const char*>(image_bytes.data()),
                 static_cast<std::streamsize>(image_bytes.size()));
    return stream.good();
#endif
}

void drawThumbnailPlaceholder(const char* i_id, const ImVec2& i_size, const std::string& i_text)
{
    ImGui::PushID(i_id);
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("thumb_placeholder", i_size);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(cursor,
                             ImVec2(cursor.x + i_size.x, cursor.y + i_size.y),
                             IM_COL32(236, 236, 236, 255),
                             8.0f);
    draw_list->AddRect(cursor,
                       ImVec2(cursor.x + i_size.x, cursor.y + i_size.y),
                       IM_COL32(188, 188, 188, 255),
                       8.0f);
    const ImVec2 text_size = ImGui::CalcTextSize(i_text.c_str());
    draw_list->AddText(ImVec2(cursor.x + (i_size.x - text_size.x) * 0.5f,
                              cursor.y + (i_size.y - text_size.y) * 0.5f),
                       IM_COL32(96, 96, 96, 255),
                       i_text.c_str());
    ImGui::PopID();
}

int64_t currentSteadyTimeMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

} // namespace

HJLiveOnLineView::HJLiveOnLineView()
{
    m_name = HJMakeGlobalName("Live OnLine");
    m_status_text = "Ready";
}

HJLiveOnLineView::~HJLiveOnLineView()
{
    HJFLogi("~HJLiveOnLineView name:{} entry", m_name);
    HJMainExecutorSync([=]() {
        stopPlayback();
    });
    HJFLogi("~HJLiveOnLineView name:{} end", m_name);
}

int HJLiveOnLineView::init(const std::string info)
{
    const int ret = HJView::init(info);
    if (ret != HJ_OK) {
        return ret;
    }

    copyStringToBuffer("", m_user_buffer.data(), m_user_buffer.size());
    copyStringToBuffer("", m_password_buffer.data(), m_password_buffer.size());
    copyStringToBuffer(kDefaultLivePageUrl,
                       m_page_url_buffer.data(),
                       m_page_url_buffer.size());
    copyStringToBuffer("", m_filter_buffer.data(), m_filter_buffer.size());
    {
        HJ_AUTO_LOCK(m_mutex);
        m_page_urls.clear();
        m_page_urls.push_back(kDefaultLivePageUrl);
        m_page_url_index = 0;
    }

    LiveCredentialConfig credential_config;
    std::string credential_error;
    if (loadDefaultCredentials(&credential_config, &credential_error)) {
        if (!credential_config.m_user.empty()) {
            copyStringToBuffer(credential_config.m_user, m_user_buffer.data(), m_user_buffer.size());
        }
        if (!credential_config.m_password.empty()) {
            copyStringToBuffer(credential_config.m_password,
                               m_password_buffer.data(),
                               m_password_buffer.size());
        }
        if (!credential_config.m_live_urls.empty()) {
            HJ_AUTO_LOCK(m_mutex);
            m_page_urls = credential_config.m_live_urls;
            m_page_url_index = 0;
            copyStringToBuffer(m_page_urls[0],
                               m_page_url_buffer.data(),
                               m_page_url_buffer.size());
        }
#if HJ_XMEDIATOOLS_HAS_LIBCURL
        setStatusText(HJFMT("Ready, credentials loaded from {}", kDefaultCredentialPath));
#else
        setStatusText(HJFMT("Ready, config loaded from {}, libcurl disabled", kDefaultCredentialPath));
#endif
    } else {
        HJ_AUTO_LOCK(m_mutex);
        m_page_urls.clear();
        m_page_url_index = -1;
#if HJ_XMEDIATOOLS_HAS_LIBCURL
        setStatusText(HJFMT("Ready, credential file unavailable: {}", credential_error));
#else
        setStatusText(HJFMT("Ready, libcurl disabled, config status: {}", credential_error));
#endif
    }
    return HJ_OK;
}

int HJLiveOnLineView::draw(const HJRectf& rect)
{
    ImGui::SetNextWindowPos({ rect.x, rect.y }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ rect.w, rect.h }, ImGuiCond_Always);

    const ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    bool is_open = m_isOpen;
    if (ImGui::Begin(HJAnsiToUtf8(m_name).c_str(), &is_open, window_flags)) {
        drawToolbar();
        ImGui::Separator();
        drawContent();
    }
    ImGui::End();

    if (m_isOpen && !is_open) {
        m_isOpen = false;
        stopPlayback();
    }
    return HJ_OK;
}

int HJLiveOnLineView::startFetch()
{
#if !HJ_XMEDIATOOLS_HAS_LIBCURL
    setStatusText("libcurl not enabled in this build", HJErrNotSupport);
    return HJErrNotSupport;
#else
    const std::string user = trimString(m_user_buffer.data());
    const std::string password = trimString(m_password_buffer.data());
    const std::string page_url = trimString(m_page_url_buffer.data());
    const std::string login_type = currentLoginType();
    const bool prefer_flv = m_prefer_flv;

    if (user.empty() || password.empty()) {
        setStatusText("username and password are required", HJErrInvalidParams);
        return HJErrInvalidParams;
    }
    if (page_url.empty()) {
        setStatusText("page url is required", HJErrInvalidParams);
        return HJErrInvalidParams;
    }

    stopPlayback();

    uint64_t session_id = 0;
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_fetching) {
            m_status_text = "fetch already running";
            m_last_result = HJErrAlreadyExist;
            return HJErrAlreadyExist;
        }
        ++m_fetch_session_id;
        session_id = m_fetch_session_id;
        m_fetching = true;
        m_cards.clear();
        m_selected_index = -1;
        m_status_text = "Fetching live resources...";
        m_last_result = HJ_OK;
    }

    HJLiveOnLineView::Wtr wself = HJSharedFromThis();
    std::thread([wself, session_id, user, password, login_type, page_url, prefer_flv]() {
        try {
            std::vector<HJLiveResourceItem> items;
            HJLiveResourceFetcher fetcher(user, password, login_type);
            fetcher.fetch(page_url, [&items](const std::vector<HJLiveResourceItem>& i_items) {
                items = i_items;
            });

            auto self = wself.lock();
            if (!self) {
                return;
            }
            self->applyFetchResult(session_id, items, prefer_flv);

            for (size_t i = 0; i < items.size(); ++i) {
                self = wself.lock();
                if (!self || !self->isFetchSessionCurrent(session_id)) {
                    return;
                }
                const std::string image_url = trimString(items[i].m_image_url);
                if (image_url.empty()) {
                    continue;
                }

                std::string cache_path;
                {
                    HJ_AUTO_LOCK(self->m_mutex);
                    if (i >= self->m_cards.size()) {
                        return;
                    }
                    cache_path = self->m_cards[i].m_thumb_cache_path;
                }

                if (cache_path.empty()) {
                    continue;
                }
                if (std::filesystem::exists(std::filesystem::path(cache_path))) {
                    self->updateThumbnailState(session_id, i, true, false, "");
                    continue;
                }

                std::string error;
                const bool ok = downloadThumbnailFile(image_url, cache_path, &error);
                self = wself.lock();
                if (!self || !self->isFetchSessionCurrent(session_id)) {
                    return;
                }
                self->updateThumbnailState(session_id, i, ok, !ok, error);
            }

            self = wself.lock();
            if (!self) {
                return;
            }
            {
                HJ_AUTO_LOCK(self->m_mutex);
                if (session_id != self->m_fetch_session_id) {
                    return;
                }
                self->m_fetching = false;
                self->m_status_text = HJFMT("Fetched {} live rooms", self->m_cards.size());
                self->m_last_result = HJ_OK;
            }
        } catch (const std::exception& exception) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            self->applyFetchError(session_id, exception.what());
        }
    }).detach();

    return HJ_OK;
#endif
}

void HJLiveOnLineView::stopPlayback()
{
    HJMediaPlayer::Ptr player{};
    HJFrameView::Ptr frame_view{};
    {
        HJ_AUTO_LOCK(m_mutex);
        ++m_playback_session_id;
        player = m_player;
        frame_view = m_frame_view;
        m_player = nullptr;
        m_video_frame = nullptr;
        m_player_media_info = nullptr;
        m_player_progress_info = nullptr;
        m_player_stream_url.clear();
        m_player_prepared = false;
        m_player_playing = false;
        m_player_muted = false;
        m_player_volume = 1.0f;
        m_frame_view = nullptr;
        resetThumbCaptureTask();
    }

    if (player != nullptr) {
        player->setMediaFrameListener(nullptr);
        player->setSourceFrameListener(nullptr);
    }
    if (frame_view != nullptr) {
        frame_view->done();
    }
}

void HJLiveOnLineView::seekPlayback(int64_t i_pos)
{
    HJMediaPlayer::Ptr player{};
    {
        HJ_AUTO_LOCK(m_mutex);
        player = m_player;
    }
    if (player == nullptr || !player->isReady()) {
        return;
    }
    player->seek(i_pos);
}

int HJLiveOnLineView::startPlayback(size_t i_index)
{
    HJLiveResourceItem item{};
    std::string stream_url;
    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index >= m_cards.size()) {
            return HJErrInvalidParams;
        }
        item = m_cards[i_index].m_item;
        stream_url = m_cards[i_index].m_stream_url;
        m_selected_index = static_cast<int>(i_index);
    }

    if (stream_url.empty()) {
        setStatusText(HJFMT("liveid {} has no playable url", item.m_liveid), HJErrNotFind);
        return HJErrNotFind;
    }

    stopPlayback();

    auto player = std::make_shared<HJMediaPlayer>();
    auto frame_view = std::make_shared<HJFrameView>();
    int ret = frame_view->init();
    if (ret != HJ_OK) {
        setStatusText(HJFMT("init frame view failed: {}", ret), ret);
        return ret;
    }

    uint64_t session_id = 0;
    {
        HJ_AUTO_LOCK(m_mutex);
        ++m_playback_session_id;
        session_id = m_playback_session_id;
        m_player = player;
        m_frame_view = frame_view;
        m_player_volume = 1.0f;
        m_player_muted = false;
        m_player_prepared = false;
        m_player_playing = false;
        m_video_frame = nullptr;
        m_player_media_info = nullptr;
        m_player_stream_url = stream_url;
    }

    HJLiveOnLineView::Wtr wself = HJSharedFromThis();
    player->setMediaFrameListener([wself, session_id, stream_url](const HJMediaFrame::Ptr i_frame) -> int {
        auto self = wself.lock();
        if (!self) {
            return HJErrAlreadyDone;
        }
        return self->onPlayerFrame(session_id, i_frame, stream_url);
    });

    auto media_url = std::make_shared<HJMediaUrl>(stream_url);
    media_url->setTimeout(5000 * 1000);
    media_url->setUseFast(false);

    ret = player->init(media_url, [wself, session_id](const HJNotification::Ptr i_ntf) -> int {
        auto self = wself.lock();
        if (!self) {
            return HJErrAlreadyDone;
        }
        return self->onPlayerNotify(session_id, i_ntf);
    });
    if (ret != HJ_OK) {
        stopPlayback();
        setStatusText(HJFMT("init player failed: {}", ret), ret);
        return ret;
    }

    setStatusText(HJFMT("opening liveid {} ...", item.m_liveid));
    return HJ_OK;
}

void HJLiveOnLineView::applyPlaybackVolume()
{
    HJMediaPlayer::Ptr player{};
    float volume = 1.0f;
    {
        HJ_AUTO_LOCK(m_mutex);
        player = m_player;
        volume = m_player_muted ? 0.0f : m_player_volume;
    }
    if (player != nullptr && player->isReady()) {
        player->setVolume(volume);
    }
}

void HJLiveOnLineView::setStatusText(const std::string& i_status, int i_result)
{
    HJ_AUTO_LOCK(m_mutex);
    m_status_text = i_status;
    m_last_result = i_result;
}

std::string HJLiveOnLineView::currentLoginType() const
{
    const int index = HJ_CLIP(m_login_type_index, 0, static_cast<int>(HJ_ARRAY_ELEMS(kLoginTypes)) - 1);
    return kLoginTypes[index];
}

std::string HJLiveOnLineView::currentFilter() const
{
    return trimString(m_filter_buffer.data());
}

bool HJLiveOnLineView::isFetchSessionCurrent(uint64_t i_session_id) const
{
    HJ_AUTO_LOCK(m_mutex);
    return i_session_id == m_fetch_session_id;
}

bool HJLiveOnLineView::isPlaybackSessionCurrent(uint64_t i_session_id) const
{
    HJ_AUTO_LOCK(m_mutex);
    return i_session_id == m_playback_session_id;
}

void HJLiveOnLineView::applyFetchResult(uint64_t i_session_id,
                                        const std::vector<HJLiveResourceItem>& i_items,
                                        bool i_prefer_flv)
{
    std::vector<LiveCardState> cards;
    cards.reserve(i_items.size());
    for (size_t i = 0; i < i_items.size(); ++i) {
        LiveCardState card;
        card.m_item = i_items[i];
        card.m_stream_url = chooseStreamUrlFor(i_prefer_flv, i_items[i]);
        card.m_thumb_cache_path = i_items[i].m_image_url.empty() ? "" : makeThumbCachePath(i_items[i].m_image_url);
        card.m_thumb_loading = !i_items[i].m_image_url.empty();
        if (!card.m_thumb_cache_path.empty() &&
            std::filesystem::exists(std::filesystem::path(card.m_thumb_cache_path))) {
            card.m_thumb_loading = false;
            card.m_thumb_ready = true;
        }
        cards.push_back(card);
    }

    HJ_AUTO_LOCK(m_mutex);
    if (i_session_id != m_fetch_session_id) {
        return;
    }
    m_cards.swap(cards);
    m_selected_index = m_cards.empty() ? -1 : 0;
    m_last_result = HJ_OK;
}

void HJLiveOnLineView::applyFetchError(uint64_t i_session_id, const std::string& i_error)
{
    HJ_AUTO_LOCK(m_mutex);
    if (i_session_id != m_fetch_session_id) {
        return;
    }
    m_fetching = false;
    m_status_text = i_error;
    m_last_result = HJErrFatal;
}

void HJLiveOnLineView::updateThumbnailState(uint64_t i_session_id,
                                            size_t i_index,
                                            bool i_ready,
                                            bool i_failed,
                                            const std::string& i_error)
{
    HJ_AUTO_LOCK(m_mutex);
    if (i_session_id != m_fetch_session_id || i_index >= m_cards.size()) {
        return;
    }

    LiveCardState& card = m_cards[i_index];
    card.m_thumb_loading = false;
    card.m_thumb_ready = i_ready;
    card.m_thumb_failed = i_failed;
    card.m_thumb_error = i_error;
    if (!i_ready) {
        card.m_thumbnail = nullptr;
    }
}

int HJLiveOnLineView::onPlayerNotify(uint64_t i_session_id, const HJNotification::Ptr& i_ntf)
{
    if (!isPlaybackSessionCurrent(i_session_id) || i_ntf == nullptr) {
        return HJErrAlreadyDone;
    }

    switch (i_ntf->getID()) {
        case HJNotify_Prepared: {
            HJMediaPlayer::Ptr player{};
            {
                HJ_AUTO_LOCK(m_mutex);
                m_player_prepared = true;
                m_player_media_info = i_ntf->getValue<HJMediaInfo::Ptr>(HJMediaInfo::KEY_WORLDS);
                player = m_player;
            }
            applyPlaybackVolume();
            if (player != nullptr) {
                HJMainExecutorAsync([player]() {
                    player->start();
                });
            }
            setStatusText("player prepared");
            break;
        }
        case HJNotify_NeedWindow: {
            HJMediaPlayer::Ptr player{};
            {
                HJ_AUTO_LOCK(m_mutex);
                player = m_player;
            }
            if (player != nullptr) {
                player->setWindow((HJHND)player.get());
            }
            setStatusText("player need window");
            break;
        }
        case HJNotify_Already: {
            HJMediaPlayer::Ptr player{};
            {
                HJ_AUTO_LOCK(m_mutex);
                m_player_playing = true;
                player = m_player;
            }
            if (player != nullptr) {
                player->play();
                applyPlaybackVolume();
            }
            setStatusText("live playing");
            break;
        }
        case HJNotify_ProgressStatus: {
            HJ_AUTO_LOCK(m_mutex);
            m_player_progress_info =
                i_ntf->getValue<HJProgressInfo::Ptr>(HJProgressInfo::KEY_WORLDS);
            break;
        }
        case HJNotify_Complete: {
            HJ_AUTO_LOCK(m_mutex);
            m_player_playing = false;
            m_status_text = "playback completed";
            break;
        }
        case HJNotify_Error: {
            const int result = i_ntf->getVal();
            HJ_AUTO_LOCK(m_mutex);
            m_player_playing = false;
            m_last_result = result;
            m_status_text = HJFMT("player error: {}", result);
            break;
        }
        default:
            break;
    }
    return HJ_OK;
}

int HJLiveOnLineView::onPlayerFrame(uint64_t i_session_id, const HJMediaFrame::Ptr& i_frame, std::string url)
{
    if (!isPlaybackSessionCurrent(i_session_id) || i_frame == nullptr) {
        return HJErrAlreadyDone;
    }
    if (!i_frame->isVideo()) {
        return HJ_OK;
    }

    HJImageWriter::Ptr image_writer{};
    std::string file_path;
    std::string output_dir;
    std::string capture_url;
    int64_t capture_time_ms = HJ_NOTS_VALUE;
    bool need_init_writer = false;
    bool need_save_frame = false;
    bool capture_completed = false;
    int saved_count = 0;
    {
        HJ_AUTO_LOCK(m_mutex);
        m_video_frame = i_frame;

        if (!m_thumb_capture_task.m_active || m_thumb_capture_task.m_url != url) {
            return HJ_OK;
        }

        capture_time_ms = i_frame->getPTS();
        if (capture_time_ms == HJ_NOTS_VALUE) {
            capture_time_ms = currentSteadyTimeMs();
        } else if (m_thumb_capture_task.m_schedule.m_has_last_saved_pts &&
                   capture_time_ms < m_thumb_capture_task.m_schedule.m_last_saved_pts_ms) {
            capture_time_ms = currentSteadyTimeMs();
        }

        if (!m_thumb_capture_task.m_schedule.shouldSaveFrame(capture_time_ms)) {
            return HJ_OK;
        }

        capture_url = m_thumb_capture_task.m_url;
        output_dir = m_thumb_capture_task.m_output_dir;
        file_path = buildThumbFilePath(kThumbOutputRootDir,
                                       m_thumb_capture_task.m_level,
                                       capture_url,
                                       m_thumb_capture_task.m_schedule.m_saved_count + 1);
        image_writer = m_thumb_capture_task.m_image_writer;
        need_init_writer = (image_writer == nullptr);
        need_save_frame = true;
    }

    if (!need_save_frame) {
        return HJ_OK;
    }

    if (need_init_writer) {
        const auto video_info = i_frame->getVideoInfo();
        if (video_info == nullptr) {
            setStatusText("thumb capture failed: missing video info", HJErrInvalidData);
            HJ_AUTO_LOCK(m_mutex);
            if (m_thumb_capture_task.m_active && m_thumb_capture_task.m_url == capture_url) {
                m_thumb_capture_task.m_hint_text = "failed";
                m_thumb_capture_task.m_hint_is_error = true;
                resetThumbCaptureTask();
            }
            return HJErrInvalidData;
        }

        auto created_writer = std::make_shared<HJImageWriter>();
        const int init_result =
            created_writer->initWithJPG(video_info->m_width, video_info->m_height, 0.8f);
        if (init_result != HJ_OK) {
            setStatusText(HJFMT("thumb capture init failed: {}", init_result), init_result);
            HJ_AUTO_LOCK(m_mutex);
            if (m_thumb_capture_task.m_active && m_thumb_capture_task.m_url == capture_url) {
                m_thumb_capture_task.m_hint_text = "failed";
                m_thumb_capture_task.m_hint_is_error = true;
                resetThumbCaptureTask();
            }
            return init_result;
        }
        image_writer = created_writer;
    }

    const int write_result = image_writer->writeFrame(i_frame, file_path);
    if (write_result != HJ_OK) {
        setStatusText(HJFMT("thumb capture write failed: {}", write_result), write_result);
        HJ_AUTO_LOCK(m_mutex);
        if (m_thumb_capture_task.m_active && m_thumb_capture_task.m_url == capture_url) {
            m_thumb_capture_task.m_hint_text = "failed";
            m_thumb_capture_task.m_hint_is_error = true;
            resetThumbCaptureTask();
        }
        return write_result;
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        if (!m_thumb_capture_task.m_active || m_thumb_capture_task.m_url != capture_url) {
            return HJ_OK;
        }
        if (m_thumb_capture_task.m_image_writer == nullptr) {
            m_thumb_capture_task.m_image_writer = image_writer;
        }
        m_thumb_capture_task.m_schedule.markSaved(capture_time_ms);
        saved_count = m_thumb_capture_task.m_schedule.m_saved_count;
        capture_completed = m_thumb_capture_task.m_schedule.isCompleted();
        if (capture_completed) {
            m_thumb_capture_task.m_hint_text = "done";
            m_thumb_capture_task.m_hint_is_error = false;
            resetThumbCaptureTask();
        } else {
            m_thumb_capture_task.m_hint_text =
                HJFMT("capturing {}/{}", saved_count, kThumbCaptureTargetCount);
            m_thumb_capture_task.m_hint_is_error = false;
        }
    }

    if (capture_completed) {
        setStatusText(HJFMT("thumb capture completed: {}", output_dir));
    } else {
        setStatusText(HJFMT("thumb saved {} / {}", saved_count, kThumbCaptureTargetCount));
    }
    return HJ_OK;
}

int HJLiveOnLineView::drawToolbar()
{
    const std::string current_page_url = trimString(m_page_url_buffer.data());

    ImGui::Text("Account");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("##live_user", m_user_buffer.data(), m_user_buffer.size());
    ImGui::SameLine();

    ImGui::Text("Password");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("##live_password",
                     m_password_buffer.data(),
                     m_password_buffer.size(),
                     ImGuiInputTextFlags_Password);
    ImGui::SameLine();

    ImGui::Text("Type");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::Combo("##live_login_type",
                 &m_login_type_index,
                 kLoginTypes,
                 static_cast<int>(HJ_ARRAY_ELEMS(kLoginTypes)));
    ImGui::SameLine();
    if (ImGui::Checkbox("Prefer FLV", &m_prefer_flv)) {
        HJ_AUTO_LOCK(m_mutex);
        for (size_t i = 0; i < m_cards.size(); ++i) {
            m_cards[i].m_stream_url = chooseStreamUrlFor(m_prefer_flv, m_cards[i].m_item);
        }
    }

    std::vector<std::string> page_urls;
    int page_url_index = -1;
    {
        HJ_AUTO_LOCK(m_mutex);
        page_urls = m_page_urls;
        page_url_index = m_page_url_index;
    }
    if (!page_urls.empty()) {
        int effective_page_index = -1;
        if (page_url_index >= 0 &&
            page_url_index < static_cast<int>(page_urls.size()) &&
            page_urls[page_url_index] == current_page_url) {
            effective_page_index = page_url_index;
        } else {
            for (size_t i = 0; i < page_urls.size(); ++i) {
                if (page_urls[i] == current_page_url) {
                    effective_page_index = static_cast<int>(i);
                    break;
                }
            }
        }

        ImGui::Text("Page");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(260.0f);
        const std::string preview_value =
            effective_page_index >= 0 ? HJFMT("Page {}", effective_page_index + 1) : "custom";
        if (ImGui::BeginCombo("##live_page_selector", preview_value.c_str())) {
            for (size_t i = 0; i < page_urls.size(); ++i) {
                const std::string page_label = HJFMT("Page {}", i + 1);
                const bool is_selected = static_cast<int>(i) == effective_page_index;
                if (ImGui::Selectable(page_label.c_str(), is_selected)) {
                    {
                        HJ_AUTO_LOCK(m_mutex);
                        m_page_url_index = static_cast<int>(i);
                    }
                    copyStringToBuffer(page_urls[i], m_page_url_buffer.data(), m_page_url_buffer.size());
                }
                if (!page_urls[i].empty()) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", page_urls[i].c_str());
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
    }

    ImGui::Text("Page Url");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-210.0f);
    if (ImGui::InputText("##live_page_url", m_page_url_buffer.data(), m_page_url_buffer.size())) {
        HJ_AUTO_LOCK(m_mutex);
        m_page_url_index = -1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fetch", ImVec2(88.0f, 0.0f))) {
        startFetch();
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh", ImVec2(88.0f, 0.0f))) {
        startFetch();
    }
#if !HJ_XMEDIATOOLS_HAS_LIBCURL
    ImGui::SameLine();
    ImGui::TextUnformatted("libcurl: disabled");
#endif

    ImGui::Text("Filter");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(260.0f);
    ImGui::InputTextWithHint("##live_filter",
                             "liveid / uid / nickname",
                             m_filter_buffer.data(),
                             m_filter_buffer.size());
    ImGui::SameLine();

    std::string status_text;
    bool fetching = false;
    int result = HJ_OK;
    size_t room_count = 0;
    {
        HJ_AUTO_LOCK(m_mutex);
        status_text = m_status_text;
        fetching = m_fetching;
        result = m_last_result;
        room_count = m_cards.size();
    }
    ImGui::Text("Rooms:%zu", room_count);
    ImGui::SameLine();
    ImGui::Text("Fetching:%s", fetching ? "yes" : "no");
    ImGui::SameLine();
    ImGui::Text("Result:%d", result);
    ImGui::TextWrapped("Status: %s", status_text.c_str());
    return HJ_OK;
}

int HJLiveOnLineView::drawContent()
{
    const ImVec2 content_size = ImGui::GetContentRegionAvail();
    const float preferred_left_width =
        kCardColumnsPreferred * kCardWidth + (kCardColumnsPreferred - 1.0f) * kCardSpacing + 24.0f;
    const float left_width = HJ_MIN(preferred_left_width, HJ_MAX(320.0f, content_size.x * 0.55f));
    const float right_width = HJ_MAX(420.0f, content_size.x - left_width - 12.0f);

    if (ImGui::BeginChild("live_cards_panel", ImVec2(left_width, content_size.y), false)) {
        drawCardGrid(ImGui::GetContentRegionAvail());
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("live_player_panel", ImVec2(right_width, content_size.y), true)) {
        drawPlayerPanel(ImGui::GetContentRegionAvail());
    }
    ImGui::EndChild();

    return HJ_OK;
}

int HJLiveOnLineView::drawCardGrid(const ImVec2& i_size)
{
    HJ_UNUSED(i_size);

    std::vector<size_t> indexes;
    std::vector<LiveCardState> cards;
    int selected_index = -1;
    {
        HJ_AUTO_LOCK(m_mutex);
        cards = m_cards;
        selected_index = m_selected_index;
    }

    const std::string filter = toLowerString(currentFilter());
    for (size_t i = 0; i < cards.size(); ++i) {
        if (matchesFilter(cards[i], filter)) {
            indexes.push_back(i);
        }
    }

    const float available_width = ImGui::GetContentRegionAvail().x;
    const int columns = HJ_MAX(1, static_cast<int>((available_width + kCardSpacing) / (kCardWidth + kCardSpacing)));
    int column_index = 0;

    for (size_t list_index = 0; list_index < indexes.size(); ++list_index) {
        const size_t card_index = indexes[list_index];
        LiveCardState card = cards[card_index];
        const bool is_selected = static_cast<int>(card_index) == selected_index;
        const ImVec4 card_bg = is_selected ? ImVec4(0.90f, 0.96f, 1.0f, 1.0f)
                                           : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        const ImVec4 text_primary = ImVec4(0.12f, 0.15f, 0.19f, 1.0f);
        const ImVec4 text_secondary = ImVec4(0.28f, 0.32f, 0.38f, 1.0f);
        const ImVec4 text_error = ImVec4(0.72f, 0.18f, 0.18f, 1.0f);

        ImGui::PushID(static_cast<int>(card_index));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, card_bg);
        ImGui::PushStyleColor(ImGuiCol_Text, text_primary);
        ImGui::PushStyleColor(ImGuiCol_Border,
                              is_selected ? ImVec4(0.32f, 0.58f, 0.86f, 1.0f)
                                          : ImVec4(0.84f, 0.86f, 0.90f, 1.0f));
        ImGui::BeginChild("live_card", ImVec2(kCardWidth, kCardHeight), true);

        if (card.m_thumb_ready && card.m_thumbnail == nullptr &&
            !card.m_thumb_cache_path.empty() &&
            std::filesystem::exists(std::filesystem::path(card.m_thumb_cache_path))) {
            card.m_thumbnail = HJGuiImageManager::getInstance()->load(card.m_thumb_cache_path);
            HJ_AUTO_LOCK(m_mutex);
            if (card_index < m_cards.size()) {
                m_cards[card_index].m_thumbnail = card.m_thumbnail;
            }
        }

        bool trigger_play = false;
        if (card.m_thumbnail != nullptr) {
            const uint64_t tex_id = card.m_thumbnail->bind();
            if (tex_id != 0) {
                if (ImGui::ImageButton("thumb", ImTextureID(tex_id), ImVec2(kThumbWidth, kThumbHeight))) {
                    trigger_play = true;
                }
            } else {
                drawThumbnailPlaceholder("thumb_bind_failed", ImVec2(kThumbWidth, kThumbHeight), "Preview");
                trigger_play = ImGui::IsItemClicked();
            }
        } else if (card.m_thumb_loading) {
            drawThumbnailPlaceholder("thumb_loading", ImVec2(kThumbWidth, kThumbHeight), "Loading...");
            trigger_play = ImGui::IsItemClicked();
        } else if (card.m_thumb_failed) {
            drawThumbnailPlaceholder("thumb_failed", ImVec2(kThumbWidth, kThumbHeight), "Thumb Failed");
            trigger_play = ImGui::IsItemClicked();
        } else {
            drawThumbnailPlaceholder("thumb_idle", ImVec2(kThumbWidth, kThumbHeight), "No Thumb");
            trigger_play = ImGui::IsItemClicked();
        }

        if (trigger_play) {
            startPlayback(card_index);
        }

        ImGui::PushStyleColor(ImGuiCol_Text, text_primary);
        ImGui::TextWrapped("%s",
                           card.m_item.m_nickname.empty()
                               ? "-"
                               : card.m_item.m_nickname.c_str());
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, text_secondary);
        ImGui::Text("LiveID: %s", card.m_item.m_liveid.empty() ? "-" : card.m_item.m_liveid.c_str());
        ImGui::Text("UID: %s", card.m_item.m_uid.empty() ? "-" : card.m_item.m_uid.c_str());
        ImGui::TextWrapped("State: %s",
                           card.m_item.m_stream_status.empty()
                               ? "-"
                               : card.m_item.m_stream_status.c_str());
        ImGui::PopStyleColor();
        if (!card.m_item.m_stream_error.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, text_error);
            ImGui::TextWrapped("Err: %s", HJAnsiToUtf8(card.m_item.m_stream_error).c_str());
            ImGui::PopStyleColor();
        }
        if (ImGui::Button("Play", ImVec2(72.0f, 0.0f))) {
            startPlayback(card_index);
        }
        ImGui::SameLine();
        if (ImGui::Button("Select", ImVec2(72.0f, 0.0f))) {
            HJ_AUTO_LOCK(m_mutex);
            m_selected_index = static_cast<int>(card_index);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ++column_index;
        if (column_index < columns && list_index + 1 < indexes.size()) {
            ImGui::SameLine();
        } else {
            column_index = 0;
        }
    }

    if (indexes.empty()) {
        ImGui::TextWrapped("No live rooms matched the current filter.");
    }
    return HJ_OK;
}

int HJLiveOnLineView::drawPlayerPanel(const ImVec2& i_size)
{
    LiveCardState selected_card{};
    bool has_selected_card = false;
    HJMediaFrame::Ptr frame{};
    HJMediaInfo::Ptr media_info{};
    HJProgressInfo::Ptr progress_info{};
    HJFrameView::Ptr frame_view{};
    HJProgressView::Ptr progress_view{};
    bool prepared = false;
    bool playing = false;
    bool muted = false;
    float volume = 1.0f;
    int selected_index = -1;
    std::string player_stream_url;
    bool thumb_capture_active = false;
    bool thumb_hint_is_error = false;
    std::string thumb_hint_text;
    {
        HJ_AUTO_LOCK(m_mutex);
        selected_index = m_selected_index;
        if (m_selected_index >= 0 && m_selected_index < static_cast<int>(m_cards.size())) {
            selected_card = m_cards[m_selected_index];
            has_selected_card = true;
        }
        frame = m_video_frame;
        media_info = m_player_media_info;
        progress_info = m_player_progress_info;
        frame_view = m_frame_view;
        progress_view = m_progress_view;
        prepared = m_player_prepared;
        playing = m_player_playing;
        muted = m_player_muted;
        volume = m_player_volume;
        player_stream_url = m_player_stream_url;
        thumb_capture_active = m_thumb_capture_task.m_active;
        thumb_hint_is_error = m_thumb_capture_task.m_hint_is_error;
        thumb_hint_text = m_thumb_capture_task.m_hint_text;
    }

    if (progress_view == nullptr) {
        auto created_progress_view = std::make_shared<HJProgressView>();
        if (created_progress_view->init() == HJ_OK) {
            HJ_AUTO_LOCK(m_mutex);
            if (m_progress_view == nullptr) {
                m_progress_view = created_progress_view;
            }
            progress_view = m_progress_view;
        }
    }

    ImGui::Text("Preview");
    ImGui::Separator();

    if (has_selected_card &&
        ImGui::CollapsingHeader("Preview Controls", ImGuiTreeNodeFlags_None)) {
        std::array<char, 2048> url_buffer{};
        copyStringToBuffer(selected_card.m_stream_url, url_buffer.data(), url_buffer.size());
        ImGui::Text("Nickname: %s",
                    selected_card.m_item.m_nickname.empty()
                        ? "-"
                        : selected_card.m_item.m_nickname.c_str());
        ImGui::SameLine();
        ImGui::Text("Prepared:%s  Playing:%s",
                    prepared ? "yes" : "no",
                    playing ? "yes" : "no");
        ImGui::Text("URL");
        ImGui::SetNextItemWidth(-96.0f);
        ImGui::InputText("##live_preview_url",
                         url_buffer.data(),
                         url_buffer.size(),
                         ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("Copy URL", ImVec2(80.0f, 0.0f))) {
            ImGui::SetClipboardText(selected_card.m_stream_url.c_str());
            setStatusText("url copied to clipboard");
        }
        if (ImGui::CollapsingHeader("Details", ImGuiTreeNodeFlags_None)) {
            ImGui::Text("LiveID:%s",
                        selected_card.m_item.m_liveid.empty() ? "-" : selected_card.m_item.m_liveid.c_str());
            ImGui::SameLine();
            ImGui::Text("UID:%s",
                        selected_card.m_item.m_uid.empty() ? "-" : selected_card.m_item.m_uid.c_str());
            if (media_info != nullptr && media_info->getVideoInfo() != nullptr) {
                auto video_info = media_info->getVideoInfo();
                ImGui::Text("Video:%dx%d codec=%d",
                            video_info->m_width,
                            video_info->m_height,
                            video_info->m_codecID);
            }
        }
        if (ImGui::Button("Play Selected", ImVec2(120.0f, 0.0f))) {
            if (selected_index >= 0) {
                startPlayback(static_cast<size_t>(selected_index));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop", ImVec2(80.0f, 0.0f))) {
            stopPlayback();
            setStatusText("playback stopped");
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Mute", &muted)) {
            {
                HJ_AUTO_LOCK(m_mutex);
                m_player_muted = muted;
            }
            applyPlaybackVolume();
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Thumb");
        ImGui::SameLine();
        if (m_thumb_level_combo == nullptr) {
            static const std::vector<std::string> kThumbLevels = { "good", "middle", "bad" };
            m_thumb_level_combo = std::make_shared<HJCombo>();
            const int init_result =
                m_thumb_level_combo->init(kThumbLevels, 0, [this](const int i_index) {
                    HJ_AUTO_LOCK(m_mutex);
                    m_thumb_capture_task.m_level_index = i_index;
                });
            if (init_result != HJ_OK) {
                m_thumb_level_combo = nullptr;
                setStatusText(HJFMT("init thumb level combo failed: {}", init_result), init_result);
            }
        }
        if (m_thumb_level_combo != nullptr) {
            m_thumb_level_combo->draw();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(thumb_capture_active);
        if (ImGui::Button("thumb", ImVec2(72.0f, 0.0f))) {
            const std::string capture_url =
                !player_stream_url.empty() ? player_stream_url : selected_card.m_stream_url;
            if (capture_url.empty()) {
                setStatusText("thumb capture failed: empty stream url", HJErrInvalidParams);
                HJ_AUTO_LOCK(m_mutex);
                m_thumb_capture_task.m_hint_text = "failed";
                m_thumb_capture_task.m_hint_is_error = true;
            } else {
                const std::string level = kThumbLevelItems[HJ_CLIP(
                    m_thumb_capture_task.m_level_index,
                    0,
                    static_cast<int>(HJ_ARRAY_ELEMS(kThumbLevelItems)) - 1)];
                const std::string output_dir = buildThumbOutputDir(kThumbOutputRootDir, level, capture_url);
                std::error_code error;
                std::filesystem::create_directories(std::filesystem::path(output_dir), error);
                if (error) {
                    setStatusText(HJFMT("create thumb dir failed: {}", error.message()),
                                  HJErrFFLoadUrl);
                    HJ_AUTO_LOCK(m_mutex);
                    m_thumb_capture_task.m_hint_text = "failed";
                    m_thumb_capture_task.m_hint_is_error = true;
                } else {
                    HJ_AUTO_LOCK(m_mutex);
                    resetThumbCaptureTask();
                    m_thumb_capture_task.m_active = true;
                    m_thumb_capture_task.m_level_index = HJ_CLIP(
                        m_thumb_capture_task.m_level_index,
                        0,
                        static_cast<int>(HJ_ARRAY_ELEMS(kThumbLevelItems)) - 1);
                    m_thumb_capture_task.m_level = level;
                    m_thumb_capture_task.m_url = capture_url;
                    m_thumb_capture_task.m_output_dir = output_dir;
                    m_thumb_capture_task.m_schedule.m_target_count = kThumbCaptureTargetCount;
                    m_thumb_capture_task.m_schedule.m_interval_ms = kThumbCaptureIntervalMs;
                    m_thumb_capture_task.m_schedule.reset();
                    m_thumb_capture_task.m_hint_text =
                        HJFMT("capturing {}/{}", 0, kThumbCaptureTargetCount);
                    m_thumb_capture_task.m_hint_is_error = false;
                    setStatusText(HJFMT("thumb capture armed: {}", output_dir));
                }
            }
        }
        ImGui::EndDisabled();
        if (!thumb_hint_text.empty()) {
            ImGui::SameLine();
            if (thumb_hint_is_error) {
                ImGui::TextColored(ImVec4(0.82f, 0.18f, 0.18f, 1.0f), "%s", thumb_hint_text.c_str());
            } else if (thumb_capture_active) {
                ImGui::TextColored(ImVec4(0.18f, 0.52f, 0.82f, 1.0f), "%s", thumb_hint_text.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.16f, 0.58f, 0.24f, 1.0f), "%s", thumb_hint_text.c_str());
            }
        }
        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.5f, "%.2f")) {
            {
                HJ_AUTO_LOCK(m_mutex);
                m_player_volume = volume;
            }
            applyPlaybackVolume();
        }
    } else {
        ImGui::TextWrapped("Select a live room on the left to preview.");
    }

    ImGui::Separator();
    int64_t progress_value = 0;
    int64_t progress_max = 0;
    if (media_info != nullptr) {
        progress_max = media_info->getDuration();
    }
    if (progress_info != nullptr) {
        progress_value = progress_info->getPos();
        if (progress_info->getDuration() > 0) {
            progress_max = progress_info->getDuration();
        }
    }
    if (progress_view != nullptr && progress_max > 0) {
        if (progress_view->draw(&progress_value, 0, progress_max)) {
            HJLiveOnLineView::Wtr wself = HJSharedFromThis();
            HJMainExecutorAsync([=]() {
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                self->seekPlayback(progress_value);
            });
        }
        ImGui::Text("Progress: %.1f%%", progress_max > 0 ? (100.0 * progress_value / progress_max) : 0.0);
        ImGui::Separator();
    } else {
        ImGui::TextUnformatted("Live stream: seek unavailable");
        ImGui::Separator();
    }

    if (ImGui::BeginChild("live_player_frame", ImVec2(0.0f, 0.0f), true)) {
        if (frame_view != nullptr && frame != nullptr) {
            const ImVec2 frame_size = ImGui::GetContentRegionAvail();
            frame_view->draw(frame, HJSizef{ frame_size.x, frame_size.y });
        } else {
            ImGui::TextWrapped("No video frame available yet.");
        }
    }
    ImGui::EndChild();

    return HJ_OK;
}

void HJLiveOnLineView::resetThumbCaptureTask()
{
    m_thumb_capture_task.m_active = false;
    m_thumb_capture_task.m_level.clear();
    m_thumb_capture_task.m_url.clear();
    m_thumb_capture_task.m_output_dir.clear();
    m_thumb_capture_task.m_schedule.reset();
    m_thumb_capture_task.m_schedule.m_target_count = kThumbCaptureTargetCount;
    m_thumb_capture_task.m_schedule.m_interval_ms = kThumbCaptureIntervalMs;
    m_thumb_capture_task.m_image_writer = nullptr;
}

bool HJLiveOnLineView::matchesFilter(const LiveCardState& i_card, const std::string& i_filter) const
{
    if (i_filter.empty()) {
        return true;
    }

    const std::string liveid = toLowerString(i_card.m_item.m_liveid);
    const std::string uid = toLowerString(i_card.m_item.m_uid);
    const std::string nickname = toLowerString(i_card.m_item.m_nickname);
    return liveid.find(i_filter) != std::string::npos ||
           uid.find(i_filter) != std::string::npos ||
           nickname.find(i_filter) != std::string::npos;
}

NS_HJ_END
