#pragma once

#if defined(__has_include)
#if __has_include(<curl/curl.h>)
#include <curl/curl.h>
#elif __has_include("../../../externals/windows/libcurl/include/curl/curl.h")
#include "../../../externals/windows/libcurl/include/curl/curl.h"
#else
#error "curl/curl.h not found"
#endif
#else
#include <curl/curl.h>
#endif

#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

NS_HJ_BEGIN
//***********************************************************************************//
struct HJLiveResourceItem {
    std::string m_page_type;
    std::string m_content_type;
    std::string m_item_type;
    std::string m_title;
    std::string m_uid;
    std::string m_liveid;
    std::string m_sn;
    std::string m_m3u8;
    std::string m_flvurl;
    std::string m_link;
    std::string m_nickname;
    std::string m_image_url;
    std::string m_stream_status;
    std::string m_stream_error;
};

class HJLiveResourceFetcher {
public:
    using FetchCallback =
        std::function<void(const std::vector<HJLiveResourceItem>& items)>;

    static std::vector<HJLiveResourceItem> parseItemsFromPageHtml(
        const std::string& page_url,
        const std::string& html);

    static std::string choosePlayableUrl(const HJLiveResourceItem& item,
                                         bool prefer_flv);

    HJLiveResourceFetcher(std::string username,
                          std::string password,
                          std::string login_type = "qihoo")
        : m_username(normalizeUsername(std::move(username), login_type)),
          m_password(std::move(password)),
          m_login_type(std::move(login_type)) {
        if (m_username.empty() || m_password.empty()) {
            throw std::runtime_error("username and password are required.");
        }
        if (m_login_type != "qihoo" && m_login_type != "system") {
            throw std::runtime_error("unsupported login_type.");
        }
    }

    void fetch(const std::string& page_url,
               const FetchCallback& callback,
               int max_pages = 1) const {
        if (!callback) {
            throw std::runtime_error("fetch callback is required.");
        }
        if (max_pages < 0) {
            throw std::runtime_error("max_pages must be >= 0.");
        }

        const std::string normalized_page_url = normalizePageUrl(page_url);
        const std::string page_type = inferPageType(normalized_page_url);
        HJScopedCurlGlobal scoped_global;
        HJScopedCurlHandle scoped_curl;
        CURL* curl = scoped_curl.get();

        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);

        ensureLoggedIn(curl, normalized_page_url);

        const std::vector<std::string> page_headers = {
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "Referer: " + buildAdminUrl("/")};
        const HJHttpResponse page_response =
            performRequest(curl, normalized_page_url, "", page_headers);

        if (page_response.m_status_code != 200) {
            throw std::runtime_error("failed to fetch page, status=" +
                                     std::to_string(page_response.m_status_code));
        }

        std::vector<HJLiveResourceItem> items =
            parseItemsFromPageHtml(normalized_page_url, page_response.m_body);
        if (!items.empty()) {
            resolveItems(curl, normalized_page_url, &items);
        } else if (isQueryPage(page_response.m_body, page_type)) {
            items = fetchQueryPages(curl, normalized_page_url, max_pages, page_type);
        } else {
            throw std::runtime_error("fetched page does not contain resource items.");
        }

        callback(items);
    }

private:
    struct HJHttpResponse {
        long m_status_code = 0;
        std::string m_body;
    };

    struct HJParsedBlock {
        size_t m_position = 0;
        std::string m_item_type;
        std::string m_start_tag;
        std::string m_block;
    };

    class HJScopedCurlGlobal {
    public:
        HJScopedCurlGlobal() {
            const CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
            if (result != CURLE_OK) {
                throw std::runtime_error("curl_global_init failed.");
            }
        }

        ~HJScopedCurlGlobal() {
            curl_global_cleanup();
        }

        HJScopedCurlGlobal(const HJScopedCurlGlobal&) = delete;
        HJScopedCurlGlobal& operator=(const HJScopedCurlGlobal&) = delete;
    };

    class HJScopedCurlHandle {
    public:
        HJScopedCurlHandle() : m_curl(curl_easy_init()) {
            if (m_curl == nullptr) {
                throw std::runtime_error("curl_easy_init failed.");
            }
        }

        ~HJScopedCurlHandle() {
            curl_easy_cleanup(m_curl);
        }

        CURL* get() const {
            return m_curl;
        }

        HJScopedCurlHandle(const HJScopedCurlHandle&) = delete;
        HJScopedCurlHandle& operator=(const HJScopedCurlHandle&) = delete;

    private:
        CURL* m_curl = nullptr;
    };

    static size_t writeCallback(char* ptr,
                                size_t size,
                                size_t nmemb,
                                void* userdata) {
        const size_t real_size = size * nmemb;
        std::string* response = static_cast<std::string*>(userdata);
        response->append(ptr, real_size);
        return real_size;
    }

    static std::string buildAdminUrl(const std::string& path) {
        return std::string(kAdminDomain) + path;
    }

    static std::string trim(const std::string& value) {
        size_t begin = 0;
        while (begin < value.size() &&
               std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
            ++begin;
        }

        size_t end = value.size();
        while (end > begin &&
               std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
            --end;
        }
        return value.substr(begin, end - begin);
    }

    static bool endsWith(const std::string& value, const std::string& suffix) {
        return value.size() >= suffix.size() &&
               value.compare(value.size() - suffix.size(),
                             suffix.size(),
                             suffix) == 0;
    }

    static std::string normalizePageUrl(const std::string& page_url) {
        const std::string trimmed_url = trim(page_url);
        if (trimmed_url.empty()) {
            throw std::runtime_error("page_url is required.");
        }
        if (trimmed_url.find("http://") == 0 ||
            trimmed_url.find("https://") == 0) {
            return trimmed_url;
        }
        if (!trimmed_url.empty() && trimmed_url[0] == '/') {
            return buildAdminUrl(trimmed_url);
        }
        return buildAdminUrl("/" + trimmed_url);
    }

    static std::string inferPageType(const std::string& page_url) {
        if (page_url.find("/Resource/Replay/") != std::string::npos) {
            return "replay";
        }
        if (page_url.find("/Resource/LiveResource/") != std::string::npos) {
            return "live_resource";
        }
        return "unknown";
    }

    static bool isSupportedPageType(const std::string& page_type) {
        return page_type == "replay" || page_type == "live_resource";
    }

    static bool containsReplayItems(const std::string& html) {
        return html.find("node-type=\"replayItem\"") != std::string::npos ||
               html.find("node-type='replayItem'") != std::string::npos;
    }

    static bool containsLinkReplayItems(const std::string& html) {
        return html.find("data-video-type=\"link-replay\"") != std::string::npos ||
               html.find("data-video-type='link-replay'") != std::string::npos;
    }

    static bool containsParsableItems(const std::string& html) {
        return containsReplayItems(html) || containsLinkReplayItems(html);
    }

    static bool containsQueryResult(const std::string& html) {
        return html.find("id=\"query_result\"") != std::string::npos ||
               html.find("id='query_result'") != std::string::npos;
    }

    static std::string buildQueryPath(const std::string& page_type) {
        if (page_type == "replay") {
            return "/Resource/Replay/postQuery";
        }
        if (page_type == "live_resource") {
            return "/Resource/LiveResource/postQuery";
        }
        return "";
    }

    static bool isQueryPage(const std::string& html, const std::string& page_type) {
        const std::string query_path = buildQueryPath(page_type);
        return !query_path.empty() &&
               html.find(query_path) != std::string::npos &&
               containsQueryResult(html);
    }

    static std::string normalizeUsername(const std::string& username,
                                         const std::string& login_type) {
        if (login_type != "qihoo") {
            return username;
        }
        const size_t at_sign = username.find('@');
        if (at_sign == std::string::npos) {
            return username;
        }
        return username.substr(0, at_sign);
    }

    static HJHttpResponse performRequest(CURL* curl,
                                         const std::string& url,
                                         const std::string& post_fields,
                                         const std::vector<std::string>& headers) {
        std::string response_body;
        struct curl_slist* header_list = nullptr;
        for (const std::string& header : headers) {
            header_list = curl_slist_append(header_list, header.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT,
                         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                         "AppleWebKit/537.36 (KHTML, like Gecko) "
                         "Chrome/145.0.0.0 Safari/537.36");

        if (!post_fields.empty()) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                             static_cast<long>(post_fields.size()));
        } else {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, nullptr);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }

        const CURLcode result = curl_easy_perform(curl);
        curl_slist_free_all(header_list);

        if (result != CURLE_OK) {
            throw std::runtime_error("curl request failed: " +
                                     std::string(curl_easy_strerror(result)));
        }

        HJHttpResponse response;
        response.m_body = response_body;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.m_status_code);
        return response;
    }

    static std::string urlEncode(CURL* curl, const std::string& value) {
        char* encoded =
            curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
        if (encoded == nullptr) {
            throw std::runtime_error("failed to URL-encode request field.");
        }
        const std::string result(encoded);
        curl_free(encoded);
        return result;
    }

    static std::string extractFirstMatch(const std::string& input,
                                         const std::vector<std::regex>& patterns,
                                         size_t group_index) {
        std::smatch match;
        for (const std::regex& pattern : patterns) {
            if (std::regex_search(input, match, pattern) &&
                group_index < match.size()) {
                return trim(match[group_index].str());
            }
        }
        return "";
    }

    static int extractPageSize(const std::string& html) {
        const std::string page_size = extractFirstMatch(
            html,
            {
                std::regex(
                    R"hj(id\s*=\s*["']page_size["'][^>]*value\s*=\s*["']([0-9]+)["'])hj",
                    std::regex::icase),
            },
            1);
        if (page_size.empty()) {
            return 50;
        }
        return std::stoi(page_size);
    }

    static std::string buildReplayPageKey(const HJLiveResourceItem& item) {
        return item.m_page_type + "|" + item.m_content_type + "|" +
               item.m_liveid + "|" + item.m_uid + "|" + item.m_sn;
    }

    static bool appendUniqueItems(const std::vector<HJLiveResourceItem>& page_items,
                                  std::set<std::string>* seen_keys,
                                  std::vector<HJLiveResourceItem>* all_items) {
        if (seen_keys == nullptr || all_items == nullptr) {
            return false;
        }

        bool has_new_item = false;
        for (const HJLiveResourceItem& item : page_items) {
            const std::string key = buildReplayPageKey(item);
            const std::pair<std::set<std::string>::iterator, bool> result =
                seen_keys->insert(key);
            if (result.second) {
                all_items->push_back(item);
                has_new_item = true;
            }
        }
        return has_new_item;
    }

    static void resolveItems(CURL* curl,
                             const std::string& page_url,
                             std::vector<HJLiveResourceItem>* items) {
        if (items == nullptr) {
            return;
        }
        for (HJLiveResourceItem& item : *items) {
            if ((item.m_flvurl.empty() || item.m_m3u8.empty()) &&
                !item.m_sn.empty()) {
                resolveStreamUrlsFromSn(curl, page_url, &item);
            }
        }
    }

    static std::string fetchQueryHtml(CURL* curl,
                                      const std::string& page_url,
                                      int page,
                                      int page_size,
                                      const std::string& page_type) {
        const std::string query_path = buildQueryPath(page_type);
        if (query_path.empty()) {
            throw std::runtime_error("unsupported resource query page type.");
        }
        const std::vector<std::string> headers = {
            "Accept: text/html, */*; q=0.01",
            "Content-Type: application/x-www-form-urlencoded; charset=UTF-8",
            "Referer: " + page_url,
            "X-Requested-With: XMLHttpRequest",
            "query-form: 1"};
        const std::string post_fields =
            "page=" + urlEncode(curl, std::to_string(page)) +
            "&page_size=" + urlEncode(curl, std::to_string(page_size));
        const HJHttpResponse response = performRequest(
            curl,
            buildAdminUrl(query_path),
            post_fields,
            headers);
        if (response.m_status_code != 200) {
            throw std::runtime_error("failed to fetch resource query html, status=" +
                                     std::to_string(response.m_status_code));
        }
        return response.m_body;
    }

    void ensureLoggedIn(CURL* curl, const std::string& page_url) const {
        const std::string login_page =
            buildAdminUrl("/account/user/login?ref=" + urlEncode(curl, page_url));
        const std::vector<std::string> login_headers = {
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "Referer: " + page_url};
        const HJHttpResponse login_page_response =
            performRequest(curl, login_page, "", login_headers);
        if (login_page_response.m_status_code != 200) {
            throw std::runtime_error("failed to open login page, status=" +
                                     std::to_string(login_page_response.m_status_code));
        }

        const std::string post_fields =
            "type=" + urlEncode(curl, m_login_type) +
            "&mail=" + urlEncode(curl, m_username) +
            "&password=" + urlEncode(curl, m_password);

        const std::vector<std::string> post_headers = {
            "Accept: application/json, text/javascript, */*; q=0.01",
            "Content-Type: application/x-www-form-urlencoded; charset=UTF-8",
            "Origin: " + std::string(kAdminDomain),
            "Referer: " + login_page,
            "X-Requested-With: XMLHttpRequest"};
        const HJHttpResponse login_response =
            performRequest(curl,
                           buildAdminUrl("/account/user/postLogin"),
                           post_fields,
                           post_headers);

        if (login_response.m_status_code != 200 ||
            login_response.m_body.find("\"code\":0") == std::string::npos) {
            throw std::runtime_error("login failed. Response body: " +
                                     login_response.m_body);
        }
    }

    static std::map<std::string, std::string> parseDataAttributes(
        const std::string& start_tag) {
        std::map<std::string, std::string> attributes;
        const std::regex data_regex(
            R"hj(data-([a-zA-Z0-9_-]+)\s*=\s*("([^"]*)"|'([^']*)'))hj",
            std::regex::icase);
        for (std::sregex_iterator iter(start_tag.begin(),
                                       start_tag.end(),
                                       data_regex);
             iter != std::sregex_iterator();
             ++iter) {
            const std::smatch& match = *iter;
            const std::string key = match[1].str();
            const std::string value =
                match[3].matched ? match[3].str() : match[4].str();
            attributes[key] = value;
        }
        return attributes;
    }

    static std::string extractImageUrl(const std::string& block) {
        const std::regex image_regex(
            R"hj(<img[^>]*src\s*=\s*("([^"]*)"|'([^']*)'))hj",
            std::regex::icase);
        std::smatch image_match;
        if (std::regex_search(block, image_match, image_regex)) {
            return image_match[2].matched ? image_match[2].str()
                                          : image_match[3].str();
        }
        return "";
    }

    static std::string buildLinkReplayNickname(const std::string& title) {
        static const char kLinkReplaySuffixBytes[] = {
            static_cast<char>(0xE7),
            static_cast<char>(0x9A),
            static_cast<char>(0x84),
            static_cast<char>(0xE8),
            static_cast<char>(0xBF),
            static_cast<char>(0x9E),
            static_cast<char>(0xE9),
            static_cast<char>(0xBA),
            static_cast<char>(0xA6),
            '\0'};
        static const std::string kLinkReplaySuffix(kLinkReplaySuffixBytes);
        if (endsWith(title, kLinkReplaySuffix)) {
            return trim(title.substr(0, title.size() - kLinkReplaySuffix.size()));
        }
        return title;
    }

    static std::vector<std::string> extractListItemTexts(
        const std::string& block) {
        std::vector<std::string> texts;
        const std::regex li_regex(R"hj(<li[^>]*>([\s\S]*?)</li>)hj",
                                  std::regex::icase);
        const std::regex tag_regex(R"hj(<[^>]+>)hj", std::regex::icase);

        for (std::sregex_iterator iter(block.begin(), block.end(), li_regex);
             iter != std::sregex_iterator();
             ++iter) {
            std::string text = (*iter)[1].str();
            text = std::regex_replace(text, tag_regex, "");
            text = trim(text);
            if (!text.empty()) {
                texts.push_back(text);
            }
        }
        return texts;
    }

    static void resolveStreamUrlsFromSn(CURL* curl,
                                        const std::string& page_url,
                                        HJLiveResourceItem* item) {
        if (item == nullptr || item->m_sn.empty() ||
            (!item->m_flvurl.empty() && !item->m_m3u8.empty())) {
            return;
        }

        const std::string request_url =
            buildAdminUrl("/resource/liveResource/livePlay?sn=" +
                          urlEncode(curl, item->m_sn));
        const std::vector<std::string> headers = {
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "Referer: " + page_url};
        const HJHttpResponse response =
            performRequest(curl, request_url, "", headers);
        if (response.m_status_code != 200) {
            item->m_stream_status = "sn_request_failed";
            item->m_stream_error =
                "livePlay request failed with status " +
                std::to_string(response.m_status_code);
            return;
        }

        const bool had_flvurl = !item->m_flvurl.empty();
        const bool had_m3u8 = !item->m_m3u8.empty();
        if (item->m_flvurl.empty()) {
            item->m_flvurl = extractFirstMatch(
                response.m_body,
                {
                    std::regex(
                        R"hj(id\s*=\s*["_']_flv_url["_'][^>]*value\s*=\s*["']([^"']+)["'])hj",
                        std::regex::icase),
                    std::regex(
                        R"hj(data-flv-url\s*=\s*["']([^"']+)["'])hj",
                        std::regex::icase),
                    std::regex(
                        R"hj((https://[^"'\\s<>]+\.flv\?[^"'\\s<>]+))hj",
                        std::regex::icase),
                },
                1);
        }

        if (item->m_m3u8.empty()) {
            item->m_m3u8 = extractFirstMatch(
                response.m_body,
                {
                    std::regex(
                        R"hj(id\s*=\s*["_']_m3u8_url["_'][^>]*value\s*=\s*["']([^"']+)["'])hj",
                        std::regex::icase),
                    std::regex(
                        R"hj(data-m3u8-url\s*=\s*["']([^"']+)["'])hj",
                        std::regex::icase),
                    std::regex(
                        R"hj((https://[^"'\\s<>]+\.m3u8[^"'\\s<>]*))hj",
                        std::regex::icase),
                },
                1);
        }

        if (!item->m_flvurl.empty() || !item->m_m3u8.empty()) {
            if (!had_flvurl || !had_m3u8) {
                item->m_stream_status = "sn_resolved";
                item->m_stream_error.clear();
            }
            return;
        }

        if (response.m_body.find("HFPlayer") != std::string::npos &&
            response.m_body.find("_flv_url") == std::string::npos &&
            response.m_body.find("data-flv-url") == std::string::npos &&
            response.m_body.find("_m3u8_url") == std::string::npos &&
            response.m_body.find("data-m3u8-url") == std::string::npos) {
            item->m_stream_status = "sn_unavailable";
            item->m_stream_error = "livePlay returned stream unavailable";
            return;
        }

        item->m_stream_status = "sn_unresolved";
        item->m_stream_error =
            "livePlay did not expose any playable stream url";
    }

    static std::vector<HJParsedBlock> collectParsedBlocks(
        const std::string& html) {
        std::vector<HJParsedBlock> parsed_blocks;

        const std::regex replay_item_regex(
            R"hj(<[^>]*node-type\s*=\s*("replayItem"|'replayItem')[^>]*>)hj",
            std::regex::icase);
        std::vector<std::pair<size_t, size_t> > item_ranges;
        for (std::sregex_iterator iter(html.begin(), html.end(), replay_item_regex);
             iter != std::sregex_iterator();
             ++iter) {
            const std::smatch& match = *iter;
            item_ranges.push_back(
                std::make_pair(static_cast<size_t>(match.position()),
                               static_cast<size_t>(match.length())));
        }

        for (size_t index = 0; index < item_ranges.size(); ++index) {
            const size_t start = item_ranges[index].first;
            const size_t start_length = item_ranges[index].second;
            const size_t next_start = index + 1 < item_ranges.size()
                                          ? item_ranges[index + 1].first
                                          : html.size();

            HJParsedBlock parsed_block;
            parsed_block.m_position = start;
            parsed_block.m_item_type = "standard";
            parsed_block.m_start_tag = html.substr(start, start_length);
            parsed_block.m_block = html.substr(start, next_start - start);
            parsed_blocks.push_back(parsed_block);
        }

        const std::regex link_replay_regex(
            R"hj(<div\s+class="fl replay_item"[^>]*>\s*<div\s+class="replay_header">\s*<div[^>]*data-video-type\s*=\s*['"]link-replay['"][\s\S]*?<ul\s+class="replay_info"[\s\S]*?</ul>\s*</div>)hj",
            std::regex::icase);
        for (std::sregex_iterator iter(html.begin(), html.end(), link_replay_regex);
             iter != std::sregex_iterator();
             ++iter) {
            const std::smatch& match = *iter;
            const std::string block = match[0].str();
            const std::string start_tag = extractFirstMatch(
                block,
                {
                    std::regex(
                        R"hj((<div[^>]*data-video-type\s*=\s*['"]link-replay['"][^>]*>))hj",
                        std::regex::icase),
                },
                1);
            if (start_tag.empty()) {
                continue;
            }

            HJParsedBlock parsed_block;
            parsed_block.m_position = static_cast<size_t>(match.position());
            parsed_block.m_item_type = "link_replay";
            parsed_block.m_start_tag = start_tag;
            parsed_block.m_block = block;
            parsed_blocks.push_back(parsed_block);
        }

        std::sort(parsed_blocks.begin(),
                  parsed_blocks.end(),
                  [](const HJParsedBlock& left, const HJParsedBlock& right) {
                      if (left.m_position != right.m_position) {
                          return left.m_position < right.m_position;
                      }
                      return left.m_item_type < right.m_item_type;
                  });
        return parsed_blocks;
    }

    static HJLiveResourceItem parseReplayBlock(const HJParsedBlock& parsed_block,
                                               const std::string& page_type) {
        const std::map<std::string, std::string> attributes =
            parseDataAttributes(parsed_block.m_start_tag);
        auto getValue = [&attributes](const std::string& key) -> std::string {
            const auto iter = attributes.find(key);
            return iter == attributes.end() ? "" : trim(iter->second);
        };

        HJLiveResourceItem item;
        item.m_page_type = page_type;
        item.m_content_type =
            page_type == "live_resource" ? "live" : "replay";
        item.m_item_type = item.m_content_type;
        item.m_uid = getValue("uid");
        item.m_liveid = getValue("liveid");
        item.m_sn = getValue("sn");
        item.m_m3u8 = getValue("m3u8");
        item.m_flvurl = getValue("flvurl");
        item.m_link = getValue("link");
        item.m_nickname = getValue("nickname");
        item.m_title = extractFirstMatch(
            parsed_block.m_block,
            {
                std::regex(
                    R"hj(<li[^>]*title\s*=\s*["']([^"']+)["'][^>]*>\s*<span[^>]*>[^<]*</span>)hj",
                    std::regex::icase),
            },
            1);
        item.m_image_url = trim(extractImageUrl(parsed_block.m_block));
        return item;
    }

    static HJLiveResourceItem parseLinkReplayBlock(
        const HJParsedBlock& parsed_block, const std::string& page_type) {
        const std::map<std::string, std::string> attributes =
            parseDataAttributes(parsed_block.m_start_tag);
        auto getValue = [&attributes](const std::string& key) -> std::string {
            const auto iter = attributes.find(key);
            return iter == attributes.end() ? "" : trim(iter->second);
        };

        HJLiveResourceItem item;
        item.m_page_type = page_type;
        item.m_content_type = "link_replay";
        item.m_item_type = "link_replay";
        item.m_sn = getValue("link-sn");
        item.m_m3u8 = getValue("link-m3u8");
        const std::vector<std::string> li_texts =
            extractListItemTexts(parsed_block.m_block);
        if (!li_texts.empty()) {
            item.m_title = li_texts[0];
        }
        if (li_texts.size() > 1) {
            std::smatch uid_match;
            if (std::regex_search(li_texts[1],
                                  uid_match,
                                  std::regex(R"hj(([0-9]{6,}))hj"))) {
                item.m_uid = uid_match[1].str();
            }
        }
        const std::string extracted_uid = extractFirstMatch(
            parsed_block.m_block,
            {
                std::regex(
                    R"hj(<li>[^<]*</li>\s*<li>[^0-9]*([0-9]+)</li>)hj",
                    std::regex::icase),
                std::regex(R"hj(UID:\s*([0-9]+))hj", std::regex::icase),
            },
            1);
        if (!extracted_uid.empty()) {
            item.m_uid = extracted_uid;
        }
        item.m_nickname = buildLinkReplayNickname(item.m_title);
        item.m_liveid = extractFirstMatch(
            parsed_block.m_block,
            {
                std::regex(
                    R"hj(/Resource/Replay/fullScreenPlay\?liveid=([0-9]+))hj",
                    std::regex::icase),
            },
            1);
        item.m_link = extractFirstMatch(
            parsed_block.m_block,
            {
                std::regex(
                    R"hj((/Resource/Replay/fullScreenPlay\?liveid=[0-9]+))hj",
                    std::regex::icase),
            },
            1);
        item.m_image_url = trim(extractImageUrl(parsed_block.m_block));
        return item;
    }

    static void finalizeItem(HJLiveResourceItem* item) {
        if (item == nullptr) {
            return;
        }
        if (!item->m_flvurl.empty() || !item->m_m3u8.empty()) {
            item->m_stream_status = "html";
        } else if (!item->m_sn.empty()) {
            item->m_stream_status = "pending_sn";
        } else {
            item->m_stream_status = "missing";
        }
    }

    static std::vector<HJLiveResourceItem> parseItemsFromHtml(
        const std::string& html, const std::string& page_type) {
        std::vector<HJLiveResourceItem> items;
        const std::vector<HJParsedBlock> parsed_blocks = collectParsedBlocks(html);
        for (const HJParsedBlock& parsed_block : parsed_blocks) {
            HJLiveResourceItem item =
                parsed_block.m_item_type == "link_replay"
                    ? parseLinkReplayBlock(parsed_block, page_type)
                    : parseReplayBlock(parsed_block, page_type);
            finalizeItem(&item);

            if (item.m_item_type == "link_replay" && item.m_liveid.empty() &&
                item.m_link.empty()) {
                continue;
            }

            if (!item.m_uid.empty() || !item.m_liveid.empty() ||
                !item.m_sn.empty() || !item.m_m3u8.empty() ||
                !item.m_flvurl.empty() || !item.m_image_url.empty()) {
                items.push_back(item);
            }
        }
        return items;
    }

    static std::vector<HJLiveResourceItem> fetchQueryPages(
        CURL* curl,
        const std::string& page_url,
        int max_pages,
        const std::string& page_type) {
        std::vector<HJLiveResourceItem> all_items;
        std::set<std::string> seen_keys;

        int current_page = 1;
        int page_size = 50;
        while (true) {
            if (max_pages > 0 && current_page > max_pages) {
                break;
            }

            const std::string list_html =
                fetchQueryHtml(curl, page_url, current_page, page_size, page_type);
            page_size = extractPageSize(list_html);

            std::vector<HJLiveResourceItem> page_items =
                parseItemsFromHtml(list_html, page_type);
            if (page_items.empty()) {
                break;
            }

            resolveItems(curl, page_url, &page_items);
            const bool has_new_item =
                appendUniqueItems(page_items, &seen_keys, &all_items);
            if (!has_new_item) {
                break;
            }
            if (static_cast<int>(page_items.size()) < page_size) {
                break;
            }
            ++current_page;
        }

        return all_items;
    }

    static constexpr const char* kAdminDomain = "https://admin.huajiao.com";

    std::string m_username;
    std::string m_password;
    std::string m_login_type;
};

inline std::vector<HJLiveResourceItem> HJLiveResourceFetcher::parseItemsFromPageHtml(
    const std::string& page_url,
    const std::string& html) {
    const std::string normalized_page_url = normalizePageUrl(page_url);
    const std::string page_type = inferPageType(normalized_page_url);
    if (!isSupportedPageType(page_type) || !containsParsableItems(html)) {
        return {};
    }
    return parseItemsFromHtml(html, page_type);
}

inline std::string HJLiveResourceFetcher::choosePlayableUrl(
    const HJLiveResourceItem& item,
    bool prefer_flv) {
    bool effective_prefer_flv = prefer_flv;
    if (item.m_page_type == "replay" || item.m_content_type == "replay" ||
        item.m_item_type == "link_replay") {
        effective_prefer_flv = false;
    } else if (item.m_page_type == "live_resource" ||
               item.m_content_type == "live") {
        effective_prefer_flv = true;
    }

    if (effective_prefer_flv) {
        if (!item.m_flvurl.empty()) {
            return item.m_flvurl;
        }
        return item.m_m3u8;
    }

    if (!item.m_m3u8.empty()) {
        return item.m_m3u8;
    }
    return item.m_flvurl;
}

NS_HJ_END
