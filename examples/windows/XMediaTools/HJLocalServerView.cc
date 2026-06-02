//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJLocalServerView.h"
#include "HJContext.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "HJLocalServer.h"
#include "HJDataSourceKit.h"
#include "HJMediaDBUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJLocalServerView::HJLocalServerView()
{
    m_name = HJMakeGlobalName("local server");
}

HJLocalServerView::~HJLocalServerView()
{
    HJFLogi("entry, name:{} entry", m_name);
    HJMainExecutorSync([=]() {
        done();
    });
    HJFLogi("end, name:{} end", m_name);
}

int HJLocalServerView::init(const std::string info)
{
    int res = HJ_OK;
    do
    {
        res = HJView::init(info);
        if (HJ_OK != res) {
            break;
        }
        //openLocalIOKit();

        m_mediaUrls = {
            "https://static.s3.huajiao.com/Object.access/hj-video/MDhmMmQwMGYyNWI5NTY0YmJkNTMzMGZiNmQwNDM1Y2VjODUxZTQzMy5tcDQ=",
            "https://file-6-huajiao.6.cn/dynamic/mpc/b0687ec0f988bd4eed8452ed02b62c07.mp4",
            "https://file-8.huajiao.com/video/c3af238a626c751c5c25669ecbff54fa.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/eHlnLm1wNA==",
            "http://static.s3.huajiao.com/Object.access/hj-video/enpxYy5tcDQ=",
            "https://file-6-huajiao.6.cn/dynamic/mpc/fc6f4d73d4066d7e96eb170f4beb361d.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/eHcubXA0",
            "http://static.s3.huajiao.com/Object.access/hj-video/eW1tbC5tcDQ=",
            "https://file-8.huajiao.com/video/4802eded4d7c8f36e53a42bd3b32469b.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/eGhuZi5tcDQ=",
            "https://file-8.huajiao.com/video/8f688d882aa2f59e51fa5daedd599397.mp4",
            "https://file-8.huajiao.com/video/369920520fe6b81143e6f32f36dcf3db.mp4",
            "https://file-8.huajiao.com/video/6b54e77b906e7f886949c95b0363949b.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/ZnN0eHh5eWIubXA0",
            "https://file-6-huajiao.6.cn/dynamic/mpc/2416d86ec4ef6bef014c85fdf235298a.mp4",
            "https://file-8.huajiao.com/video/605c230add69e5d226a4c48e62582fb3.mp4",
            "https://file-8.huajiao.com/video/2dfe15c8bf9b929349395ee245d8e2d5.mp4",
            "https://file-8.huajiao.com/video/9dd37507b4897189d1264d834f7eb10c.mp4",
            "https://file-11.huajiao.com/common_operations/36f3e2fb26a0fc5901b4d8b8bd410dcf.mp4",
            "https://file-8.huajiao.com/video/e004973282feba10790acb18cbf20d69.mp4",
            "https://file-8.huajiao.com/video/840cdc62c6c319b73dca26d0d996cb31.mp4",
            "https://file-8.huajiao.com/video/c234ca43a51f476014f4a9cb4b9f53c9.mp4",
            "https://file-8.huajiao.com/video/9fffe824005c1af50742e1a4a1b49844.mp4",
            "https://file-8.huajiao.com/video/69faab957ed0dbb98d38be1812a03ea4.mp4",
            "https://file-8.huajiao.com/video/e994c9ec44ab28a97923b87a0ee3041b.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/d2p4eGEubXA0",
            "http://static.s3.huajiao.com/Object.access/hj-video/eGhuZmEubXA0",
            "http://static.s3.huajiao.com/Object.access/hj-video/enp5Y2EubXA0",
            "https://file-8.huajiao.com/video/11dc579abab2cc79a57cfe3bfc45333b.mp4",
            "https://static.s3.huajiao.com/Object.access/hj-video/MTdjOTIwZWEwMzQ1NTI3OGIxNTc0MzVkNmI0NjY1YjQ5MDQ2MDc2OS5tcDQ=",
            "https://file-8.huajiao.com/video/b9b90c32d02ba4641b01951d5375c3a5.mp4",
            "https://file-8.huajiao.com/video/61ff6960ffd98bf0e0c2089bbceecf06.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/MGYwODdhNzk3NWY2N2NmMDJmY2JlZjY0YTdmNmNmZTVhYWQyZmM2Ny5tcDQ=",
            "http://static.s3.huajiao.com/Object.access/hj-video/bG15aC5tcDQ=",
            "https://file-8.huajiao.com/video/55a55789b0ab3626794921b24b230c6e.mp4",
            "https://file-8.huajiao.com/video/42d2984e7269b08598a581892334af15.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/RmVpWmFvLm1wNA==",
            "https://file-11.huajiao.com/common_operations/432fa6aec5cf61a6a09e817a9f7c3df2.mp4",
            "https://file-8.huajiao.com/video/f100a66da17683e9cc7fb98dfc58e749.mp4",
            "https://file-11.huajiao.com/common_operations/ed14b39e6108cf73355a2a99f7c4f72d.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/dGVjYi5tcDQ=",
            "https://file-8.huajiao.com/video/8f688d882aa2f59e51fa5daedd599397.mp4&level=1",
            "http://wm-video-huajiao.6.cn/vod-wm-huajiao-bj/248399537_4-1712297044-d5dc6762-f4ea-efb5.mp4",
            "https://file-6-huajiao.6.cn/dynamic/mpc/b1d65b1a37d27fcee77ed0a7b7b072e5.mp4",
            "https://file-6-huajiao.6.cn/dynamic/mpc/494028d41535d497866bd120e6a2704b.mp4",
            "https://file-6-huajiao.6.cn/dynamic/mpc/1bd09f110b833dd0604d008a4d206b61.mp4",
            "http://wm-video-huajiao.6.cn/vod-wm-huajiao-bj/249117731_4-1728557537-c9a0f703-6f4d-ca5f.mp4",
            "https://file-6-huajiao.6.cn/dynamic/mpc/db7a8c3d740058f1246831fe9a03bee2.mp4",
            "http://static.s3.huajiao.com/Object.access/hj-video/eWJiYi5tcDQ="
        };

        for (const auto& url : m_mediaUrls) {
            auto btn = std::make_shared<HJComplexImageButton>();
            HJLocalServerView::Wtr wself = HJSharedFromThis();
            btn->init({ { u8"down", "ic_download.png" }, {u8"pause", "ic_pause.png"} }, { 36.0f, 36.0f }, [wself, url](const std::string tips) {
                auto tself = wself.lock();
                if (!tself) {
                    return;
                }
                tself->onClickDownload(tips, url);
            });
            m_downloadButtons.push_back(btn);
        }
        for (const auto& url : m_mediaUrls) {
            HJLocalServerView::Wtr wself = HJSharedFromThis();
            auto btn = std::make_shared<HJComplexImageButton>();
            btn->init({ { u8"play", "ic_play2.png" }, {u8"pause", "icon_pause.png"} }, { 36.0f, 36.0f }, [wself, url](const std::string tips) {
                auto tself = wself.lock();
                if (!tself) {
                    return;
                }
                tself->onClickPlay(tips, url);
            });
            m_playButtons.push_back(btn);
        }
    } while (false);

    return res;
}

void HJLocalServerView::done()
{
    closeLocalIOKit();
}

int HJLocalServerView::draw(const HJRectf& rect)
{
    int res = HJ_OK;
    //HJFLogi("entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
    do
    {
        res = drawStatusBar(rect);
        if (HJ_OK != res) {
            break;
        }
        drawDBInfo(rect);
        drawMediaList(rect);
    } while (false);

    return res;
}

int HJLocalServerView::drawStatusBar(const HJRectf& rect)
{
    int res = HJ_OK;
    //HJFLogi("draw entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
    do
    {
        const ImGuiIO& io = ImGui::GetIO();
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        //
        ImGui::SetNextWindowPos({ rect.x, rect.y });
        //ImGui::SetNextWindowSize({ rect.w, rect.h });
        ImGui::SetNextWindowSize({ rect.w, 1.5f * rect.y });
        ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background
        if (ImGui::Begin(HJAnsiToUtf8(m_name + "statusbar").c_str(), &m_isOpen, window_flags))
        {
            ImGui::SameLine();
            ImGui::Checkbox("Category Infos", &m_showDBInfoWindow);

            //close
            float spacing_w = ImGui::GetWindowSize().x - 7 * style.ItemSpacing.x;
            ImGui::SameLine(spacing_w, 0.0f);
            if (!m_btnClose) {
                m_btnClose = std::make_shared<HJImageButton>();
                res = m_btnClose->init("ic_close.png", "close player view", { HJView::K_ICON_SIZE_W_DEFAULT, HJView::K_ICON_SIZE_H_DEFAULT }, [&]() {
                    m_isOpen = false;
                    });
                if (HJ_OK != res) {
                    HJLoge("error, init close button failed");
                    return res;
                }
            }
            if (m_btnClose) {
                m_btnClose->draw();
            }
        }
        ImGui::End();
    } while (false);

    return res;
}

int HJLocalServerView::drawDBInfo(const HJRectf& rect)
{
    if (!m_showDBInfoWindow) {
        m_categroyInfos = nullptr;
        return HJErrNotAlready;
    }
    if(!m_categroyInfos) {
        m_categroyInfos = std::make_shared<std::vector<HJDBCategoryInfo>>();
        auto categoryInfos = HJDataSourceKit::getInstance()->getAllCategoryInfos();
        *m_categroyInfos = categoryInfos;
    }
    if(m_categroyInfos->size() <= 0) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    //HJFLogi("draw entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
    do
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::SetNextWindowPos({ rect.x + 0.5f * rect.w, 3.0f * rect.y}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.3f);

        if (ImGui::Begin(HJAnsiToUtf8("category infos").c_str(), &m_showDBInfoWindow, window_flags))
        {
            for (const auto& info : *m_categroyInfos) {
                ImGui::Text("Name: %s", info.name.c_str());
                ImGui::Text("Type: %d", info.type);
                ImGui::Text("Max Size: %lld", info.max_size);
                ImGui::Text("File Count: %d", info.file_count);
                ImGui::Text("Total Size: %lld", info.total_size);
                ImGui::Text("Local Dir: %s", HJAnsiToUtf8(info.local_dir).c_str());
                ImGui::Separator();
            }
        }
        ImGui::End();
    } while (false);

    return res;
}

int HJLocalServerView::drawMediaList(const HJRectf& rect)
{
    int res = HJ_OK;
    do
    {
        if (m_mediaUrls.empty()) {
            break;
        }

        const ImGuiIO& io = ImGui::GetIO();
        const ImGuiStyle& style = ImGui::GetStyle();
        // Remove NoInputs and AlwaysAutoResize for better interaction and stability
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        //
        float offset_y = 2.5f * rect.y;
        ImGui::SetNextWindowPos({ rect.x, offset_y });
        ImGui::SetNextWindowSize({ rect.w, rect.h - offset_y });
        ImGui::SetNextWindowBgAlpha(0.8f);

        if (ImGui::Begin(HJAnsiToUtf8("media list").c_str(), nullptr, window_flags))
        {
            int i = 0;
            float icon_w = 46.0f;
            float status_w = 40.0f;
            float spacing = style.ItemSpacing.x;
            
            for (const auto& url : m_mediaUrls) {
                ImGui::PushID(i);
                
                // Get available width at the start of each row to account for potential scrollbar
                float avail_w = ImGui::GetContentRegionAvail().x;

                // 1. Icon
                if (i < m_downloadButtons.size()) {
                    m_downloadButtons[i]->draw();
                }
                
                ImGui::SameLine();
                
                // 2. Info Group
                float info_w = avail_w - icon_w - status_w - 2 * spacing;
                if (info_w < 100.0f) info_w = 100.0f;

                ImGui::BeginGroup();
                // Filename
                //std::string filename = url;
                //size_t last_slash = url.find_last_of('/');
                //if (last_slash != std::string::npos) {
                //    filename = url.substr(last_slash + 1);
                //}
                //ImGui::Text("%s", HJAnsiToUtf8(filename).c_str());
                ImGui::Text(url.c_str());
                
                // Progress Bar
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor(66, 150, 250));
                float progress = 0.0f;
                auto itemInfo = getMediaItemInfo(url);
                if(itemInfo.has_value()) {
                    progress = itemInfo->valid_size / (float)itemInfo->total_length;
                }
                ImGui::ProgressBar(progress, ImVec2(info_w, 4), "");
                ImGui::PopStyleColor();
                
                // Size status
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(150, 150, 150));
                std::string fileStatusStr = "0 / 0";
                if (itemInfo.has_value()) {
                    fileStatusStr = HJFMT("{} / {}", itemInfo->valid_size, itemInfo->total_length);
                }
                ImGui::Text(fileStatusStr.c_str());
                ImGui::PopStyleColor();
                ImGui::EndGroup();

                ImGui::SameLine();
                
                // 3. Status/Action - Force Right Alignment
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail_w - ImGui::GetCursorPosX() - status_w + spacing));
                ImGui::BeginGroup();
                if (i < m_playButtons.size()) {
                    m_playButtons[i]->draw();
                }
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(pos.x + status_w * 0.5f, pos.y + 18.0f), 4.0f, ImColor(150, 150, 150));
                ImGui::Dummy(ImVec2(status_w, 36)); // Occupy space
                ImGui::EndGroup();

                ImGui::Separator();
                ImGui::PopID();
                i++;
            }
        }
        ImGui::End();
    } while (false);

    return res;
}

//***********************************************************************************//
void HJLocalServerView::onClickDownload(const std::string& tips, const std::string& url)
{
    HJLocalServerView::Wtr wself = HJSharedFromThis();
    HJMainExecutorAsync(([wself, tips, url]() {
        HJLocalServerView::Ptr tself = wself.lock();
        if (!tself) {
            return;
        }
        if ("down" == tips) {
            tself->downloadMedia(url);
        } else { //cancel
            tself->cancelMedia(url);
        }
    }));
}

void HJLocalServerView::onClickPlay(const std::string& tips, const std::string& url)
{
    HJLocalServerView::Wtr wself = HJSharedFromThis();
    HJMainExecutorAsync(([wself, tips, url]() {
        HJLocalServerView::Ptr tself = wself.lock();
        if (!tself) {
            return;
        }
        if ("play" == tips) {
            tself->playMedia(url);
        }
        else { //cancel
            tself->stopMedia(url);
        }
        }));
}

//***********************************************************************************//
int HJLocalServerView::openLocalIOKit()
{
    HJParams::Ptr params = HJCreates<HJParams>();

    HJCacheOptions cache_opts = {
        { "E:/movies/localio/medias", 50},
        { "E:/movies/localio/short", 200},
        { "E:/movies/localio/gift", 300 }
    };
    (*params)["cache_opts"] = cache_opts;
    (*params)["medias_dir"] = "E:/movies/localio";
    (*params)["medias_cache_max"] = 500;

    HJDataSourceKit::getInstance()->init(params, [&](const HJNotification::Ptr ntf) -> int {
        procMediaNotify(ntf);
        return HJ_OK;
    });

    return HJ_OK;
}

void HJLocalServerView::closeLocalIOKit()
{
    HJDataSourceKit::getInstance()->done();
}

int HJLocalServerView::procMediaNotify(const HJNotification::Ptr& ntf)
{
    if (!ntf) {
        return HJ_OK;
    }
    switch (ntf->getID()) {
    case HJ_LOCALIO_NOTIFY_CACHE_START: {
        auto url = ntf->getString("url");
        auto rid = ntf->getString("rid");
        auto total_size = ntf->getInt64("total");
        auto valid_size = ntf->getInt64("valid");
        updateMediaItemInfo(url, rid, valid_size, total_size);
        HJFLogi("local io open: {}, size: {} / {}", ntf->getMsg(), valid_size, total_size);
        break;
    }
    case HJ_LOCALIO_NOTIFY_CACHE_PROGRESS: {
        auto url = ntf->getString("url");
        auto rid = ntf->getString("rid");
        auto progress = ntf->getVal();
        auto total_size = ntf->getInt64("total");
        auto valid_size = ntf->getInt64("valid");
        updateMediaItemInfo(url, rid, valid_size, total_size);
        HJFLogi("local io progress: {}, {}, size: {} / {}", ntf->getVal(), ntf->getMsg(), valid_size, total_size);
        break;
    }
    case HJ_LOCALIO_NOTIFY_CACHE_COMPLETED: {
        auto url = ntf->getString("url");
        auto rid = ntf->getString("rid");
        auto result = ntf->getVal();
        auto total_size = ntf->getInt64("total");
        auto valid_size = ntf->getInt64("valid");
        updateMediaItemInfo(url, rid, valid_size, total_size);
        HJFLogi("local io completed: {}, size: {} / {}", ntf->getMsg(), valid_size, total_size);
        break;
    }
    case HJ_LOCALIO_NOTIFY_CACHE_FAILED: {
        auto url = ntf->getString("url");
        auto rid = ntf->getString("rid");
        auto result = ntf->getVal();
        auto total_size = ntf->getInt64("total");
        auto valid_size = ntf->getInt64("valid");
        updateMediaItemInfo(url, rid, valid_size, total_size, result);
        HJFLogi("local io failed: {}, size: {} / {}", ntf->getMsg(), valid_size, total_size);
        break;
    }
    };

    return HJ_OK;
}

//***********************************************************************************//
int HJLocalServerView::downloadMedia(const std::string& url)
{
    std::string rid = HJMediaUtils::makeUrlRid(url);
    std::string local_dir = "E:/movies/localio/medias";
    int preCacheSize = -1; //1 * 1024 * 1024;
    auto ret_rid = HJDataSourceKit::getInstance()->download(url, local_dir, rid, preCacheSize, 0, [&](const HJNotification::Ptr ntf) -> int {
        return procMediaNotify(ntf);
    });

    return HJ_OK;
}

int HJLocalServerView::cancelMedia(const std::string& url)
{
    auto rid = HJMediaUtils::makeUrlRid(url);
    HJDataSourceKit::getInstance()->cancel(rid);

    return HJ_OK;
}

int HJLocalServerView::playMedia(const std::string& url)
{
    return HJ_OK;
}
int HJLocalServerView::stopMedia(const std::string& url)
{
    return HJ_OK;
}

NS_HJ_END