//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJPlayerView.h"
#include "HJContext.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "HJLocalServer.h"
#include "HJM3U8Parser.h"
//#include "HJByteBuffer.h"
//#include "IconFontCppHeaders/IconsFontAwesome5.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJPlayerView::HJPlayerView()
{
    m_name = HJMakeGlobalName("Stream Pusher");
}

HJPlayerView::~HJPlayerView()
{
    HJFLogi("~HJPlayerView name:{} entry", m_name);
    HJMainExecutorSync([=]() {
        done();
    });
    HJFLogi("~HJPlayerView name:{} end", m_name);
}

int HJPlayerView::init(const std::string info)
{
    int res = HJ_OK;
    do
    {
        res = HJView::init(info);
        if (HJ_OK != res) {
            break;
        }
        //m_mediaUrl = HJContext::Instance().getMediaDir() + "/877312_H264_720P_10M.mp4";
        //m_mediaUrl = HJContext::Instance().getMediaDir() + "/�ز�/720P/877312_H264_720P_10M.mp4";
        //m_mediaUrl = "https://al2-flv.live.huajiao.com/live_huajiao_h265/_LC_AL2_non_h265_SD_27009459017186963470016983_OX.flv";
        //m_mediaUrl = "E:/movies/qiyiboshi2.mp4";
        //m_mediaUrl = "E:/js/1718781990_2.ts";
#if defined(HJ_OS_WIN32)
        //m_mediaUrl = "E:/movies/720x1280.mp4";
        //m_mediaUrl = "https://wlive.6rooms.com/httpflv/v101369809-220734480.flv";
        //m_mediaUrl = "E:/movies/h265_548637.flv";
        //m_mediaUrl = "https://live-replay-5.test.huajiao.com/psegments/z1.huajiao-live.HJ_0_qiniu_5_huajiao-live__h265_45752749_1730776617184_3550_T/1730776662-1730776884.m3u8";
        //m_mediaUrl = "https://file-21.huajiao.com/record/main/HJ_0_ali_2_main__h265_271891644_1751257106161_8625_O/replay.m3u8"; 
        //m_mediaUrl = "https://live-pull-1.huajiao.com/main/HJ_0_ali_1_main__h265_268302111_1761034596728_7704_O.flv?auth_key=1761121005-0-0-c496e8d2513f9ac4790f04c9a54d5a45";
        //m_mediaUrl = "E:/movies/replaym3u8/index.m3u8";
        //m_mediaUrl = "https://live-pull-2.huajiao.com/main/HJ_0_ali_2_main_a_h264_270100274_1732602713389_8267_O.flv?auth_key=1732694308-0-0-d6f19de17979da5df6536153270dc20f";
        //m_mediaUrl = "https://al2-flv.live.huajiao.com/live_huajiao_h265/_LC_AL2_non_h265_SD_26624183417212690010010622_OX.flv";//"E:/js/820827.crdownload.flv";
        //m_mediaUrl = "https://live-pull-3.huajiao.com/main/HJ_0_tc_3_main__h265_268923734_1764076825048_2303_O.flv?txSecret=fcf8d8c2eb3a399c2b727e3bf21f7a02&txTime=6927B9EA";
        //m_mediaUrl = "https://live-pull-7.test.huajiao.com/main/HJ_0_ws_7_main_a_h264_41000654_1764295166192_6823_T.flv?wsSecret=23cac128f591429b85d3685403f2f32e&wsTime=1764385363";
        m_mediaUrl = "https://live-pull-2.huajiao.com/main/HJ_0_ali_2_main__h265_271533083_1765441687849_4660_O.flv?auth_key=1765528429-0-0-cd854117f70993c49feaacd40dd432f3";
        //m_mediaUrl = "http://localhost:8080/live/livestream.flv";
        //m_mediaUrl = "E:/movies/blob/server/20210325.mp4";
        //m_mediaUrl = "E:/movies/blob/server/2024_08_19_11_01_48_371_783.flv";
        //m_mediaUrl = "E:/movies/blob/server/zzqc.mp4";
        //m_mediaUrl = "E:/movies/blob/server/oceans.mp4";
        //m_mediaUrl = "http://vjs.zencdn.net/v/oceans.mp4";
        //m_mediaUrl = "https://file-21.huajiao.com/record/main/HJ_0_ali_2_main__h265_271728824_1764727312954_8854_O/replay.m3u8";
        //m_mediaUrl = "https://file-22.huajiao.com/record/main/HJ_0_ali_1_main__h265_271393148_1764735837857_3557_O/replay.m3u8";
        //m_mediaUrl = "https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8";
        //m_mediaUrl = "http://devimages.apple.com/iphone/samples/bipbop/gear1/prog_index.m3u8";
#elif defined(HJ_OS_MACOS)
        m_mediaUrl = "/Users/zhiyongleng/works/movies/720x1280.mp4";
#endif
        m_thumbUrl = HJUtilitys::exeDir() + "thumbs/";

        HJFLogi("init url:{}", m_mediaUrl);
    } while (false);

    return res;
}

void HJPlayerView::done()
{
    HJ_AUTO_LOCK(m_mutex);
    m_progInfo = nullptr;
    m_minfo = nullptr;
    m_minfos.clear();
    m_mvf = nullptr;
    m_player = nullptr;
    m_curPos = 0;

    //m_imageWriter = nullptr;
    m_imageIdx = 0;

    if (m_muxer) {
        m_muxer->done();
        m_muxer = nullptr;
    }
    //m_pusher = nullptr;
    if (m_rtmpMuxer) {
		m_rtmpMuxer->setQuit();
        m_rtmpMuxer->done();
        m_rtmpMuxer = nullptr;
    }
    //
//    m_mvfView = nullptr;
}

int HJPlayerView::drawMuxerStat(const HJRectf& rect)
{
    if (m_rtmpMuxer)
    {
        HJJPBPlotInfos plotInfos;
        auto params = m_rtmpMuxer->getStats();
        if (m_muxerStatParams.size() > 5000) {
            m_muxerStatParams.erase(m_muxerStatParams.begin());
        }
        m_muxerStatParams.push_back(params);
        //
        double xsMin = INT64_MAX;
        double xsMax = 0;
        double ysMax = 0;
        int64_t sampleCount = m_muxerStatParams.size();
        int64_t sampleMax = HJ_MIN(3 * 1000, m_muxerStatParams.size());

        HJBuffer::Ptr xsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr ysbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr kbpsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr fpsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr netkbpsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr delaybuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        float* xs = (float*)xsbuffer->data();
        float* ys = (float*)ysbuffer->data();
        float* kbps = (float*)kbpsbuffer->data();
        float* fps = (float*)fpsbuffer->data();
        float* netkbps = (float*)netkbpsbuffer->data();
        float* delay = (float*)delaybuffer->data();

        size_t j = HJ_MAX(0, sampleCount - sampleMax);
        for (size_t i = 0; i < sampleMax; i++, j++)
        {
            const auto& val = m_muxerStatParams[j];
            xs[i] = val.time;
            if (xs[i] > xsMax) {
                xsMax = xs[i];
            }
            if (xs[i] < xsMin) {
                xsMin = xs[i];
            }
            ys[i] = val.cacheSize;
            if (ys[i] > ysMax) {
                ysMax = ys[i];
            }
            kbps[i] = val.outStats.video_kbps / 100.0;
            if (kbps[i] > ysMax) {
                ysMax = kbps[i];
            }
            fps[i] = val.outStats.video_fps;
            if (fps[i] > ysMax) {
                ysMax = fps[i];
            }
            delay[i] = val.delay;
            if (delay[i] > ysMax) {
                ysMax = delay[i];
            }
            netkbps[i] = val.inStats.video_kbps / 100.0;
            if (netkbps[i] > ysMax) {
                ysMax = netkbps[i];
            }
            //lower[i] = val->rlimit;
            ////
            //speed[i] = val->speed;
            //stutter[i] = val->stut * 10.0;
        }

        plotInfos.xsbuffer = xsbuffer;
        plotInfos.xsMin = xsMin;
        plotInfos.xsMax = std::fmax(xsMax, 1000.0);
        plotInfos.ysbuffers["dura"] = ysbuffer;
        plotInfos.ysMaxs["dura"] = ysMax;
        plotInfos.size = sampleMax;
        plotInfos.ysbuffers["kbps"] = kbpsbuffer;
        plotInfos.ysbuffers["fps"] = fpsbuffer;
        plotInfos.ysbuffers["delay"] = delaybuffer;
        plotInfos.ysbuffers["netkpbs"] = netkbpsbuffer;
        //
        float m_fillRefValue = 0.0f;
        bool show_fills = false;
        ImGui::SameLine();
        if (plotInfos.xsbuffer && ImPlot::BeginPlot("pusher params Curve", ImVec2(-1, 0.6 * rect.h)))
        {
            ImPlot::SetupAxes("ms", "kbps");
            //ImPlot::SetupAxesLimits(0, xsMax, 0, 1.5 * ysMax);
            double ysMax = plotInfos.ysMaxs.begin()->second;
            HJBuffer::Ptr ysbuffer = nullptr;
            auto it = plotInfos.ysbuffers.find("dura");
            if (it != plotInfos.ysbuffers.end()) {
                ysbuffer = it->second;
            }
            float* xs = (float*)plotInfos.xsbuffer->data();
            float* ys = (float*)ysbuffer->data();
            //
            //HJBuffer::Ptr avgbuffer = plotInfos.ysbuffers.find("avg")->second;
            //float* avg = (float*)avgbuffer->data();
            ////HJBuffer::Ptr maxbuffer = plotInfos.ysbuffers.find("max")->second;
            ////float* max = (float*)maxbuffer->data();
            ////HJBuffer::Ptr minbuffer = plotInfos.ysbuffers.find("min")->second;
            ////float* min = (float*)minbuffer->data();
            //HJBuffer::Ptr upperbuffer = plotInfos.ysbuffers.find("upper")->second;
            //float* upper = (float*)upperbuffer->data();
            //HJBuffer::Ptr lowerbuffer = plotInfos.ysbuffers.find("lower")->second;
            //float* lower = (float*)lowerbuffer->data();

            ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, plotInfos.xsMin, plotInfos.xsMax, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.2 * ysMax, ImGuiCond_Always);
            if (show_fills) {
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                ImPlot::PlotShaded("queue shaded", xs, ys, plotInfos.size, m_fillRefValue);
                ImPlot::PopStyleVar();
            }
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f, 0.95f, 0.2f, 1.0f));
            ImPlot::PlotLine("queue", xs, ys, plotInfos.size);
            ImPlot::PopStyleColor();
            ImPlot::PlotLine("100kbps", xs, kbps, plotInfos.size);
            ImPlot::PlotLine("fps", xs, fps, plotInfos.size);
            ImPlot::PlotLine("delay", xs, delay, plotInfos.size);
            ImPlot::PlotLine("net 10Mbps", xs, netkbps, plotInfos.size);

            //ImPlot::PlotLine("duration", xs, max, plotInfos.size);
            //ImPlot::PlotLine("duration", xs, min, plotInfos.size);
            //ImPlot::PlotLine("upper", xs, upper, plotInfos.size);
            //ImPlot::PlotLine("lower", xs, lower, plotInfos.size);

            //
            ImPlot::EndPlot();
        }
    }
    return HJ_OK;
}

int HJPlayerView::drawPusherStat(const HJRectf& rect)
{
#if 0
    if (m_pusher) 
    {
        HJJPBPlotInfos plotInfos;
        auto params = m_pusher->getStatParams();
        if (m_jpStatParams.size() > 5000) {
            m_jpStatParams.erase(m_jpStatParams.begin());
        }
        m_jpStatParams.push_back(params);
        //
        double xsMin = INT64_MAX;
        double xsMax = 0;
        double ysMax = 0;
        int64_t sampleCount = m_jpStatParams.size();
        int64_t sampleMax = HJ_MIN(3 * 1000, m_jpStatParams.size());

        HJBuffer::Ptr xsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr ysbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr kbpsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr fpsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr netkbpsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        HJBuffer::Ptr delaybuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
        float* xs = (float*)xsbuffer->data();
        float* ys = (float*)ysbuffer->data();
        float* kbps = (float*)kbpsbuffer->data();
        float* fps = (float*)fpsbuffer->data();
        float* netkbps = (float*)netkbpsbuffer->data();
        float* delay = (float*)delaybuffer->data();

        size_t j = HJ_MAX(0, sampleCount - sampleMax);
        for (size_t i = 0; i < sampleMax; i++, j++)
        {
            const auto& val = m_jpStatParams[j];
            xs[i] = val.time;
            if (xs[i] > xsMax) {
                xsMax = xs[i];
            }
            if (xs[i] < xsMin) {
                xsMin = xs[i];
            }
            ys[i] = val.queue_length;
            if (ys[i] > ysMax) {
                ysMax = ys[i];
            }
            kbps[i] = val.sent_bps / 100000.0;
            if (kbps[i] > ysMax) {
                ysMax = kbps[i];
            }
            fps[i] = val.sent_fps;
            if (fps[i] > ysMax) {
                ysMax = fps[i];
            }
            delay[i] = val.push_delay;
            if (delay[i] > ysMax) {
                ysMax = delay[i];
            }
            netkbps[i] = val.net_kbps / 10000.0;
            if (netkbps[i] > ysMax) {
                ysMax = netkbps[i];
            }
            //lower[i] = val->rlimit;
            ////
            //speed[i] = val->speed;
            //stutter[i] = val->stut * 10.0;
        }

        plotInfos.xsbuffer = xsbuffer;
        plotInfos.xsMin = xsMin;
        plotInfos.xsMax = std::fmax(xsMax, 1000.0);
        plotInfos.ysbuffers["dura"] = ysbuffer;
        plotInfos.ysMaxs["dura"] = ysMax;
        plotInfos.size = sampleMax;
        plotInfos.ysbuffers["kbps"] = kbpsbuffer;
        plotInfos.ysbuffers["fps"] = fpsbuffer;
        plotInfos.ysbuffers["delay"] = delaybuffer;
        plotInfos.ysbuffers["netkpbs"] = netkbpsbuffer;
        //
        float m_fillRefValue = 0.0f;
        bool show_fills = false;
        ImGui::SameLine();
        if (plotInfos.xsbuffer && ImPlot::BeginPlot("pusher params Curve", ImVec2(-1, 0.6 * rect.h)))
        {
            ImPlot::SetupAxes("ms", "kbps");
            //ImPlot::SetupAxesLimits(0, xsMax, 0, 1.5 * ysMax);
            double ysMax = plotInfos.ysMaxs.begin()->second;
            HJBuffer::Ptr ysbuffer = nullptr;
            auto it = plotInfos.ysbuffers.find("dura");
            if (it != plotInfos.ysbuffers.end()) {
                ysbuffer = it->second;
            }
            float* xs = (float*)plotInfos.xsbuffer->data();
            float* ys = (float*)ysbuffer->data();
            //
            //HJBuffer::Ptr avgbuffer = plotInfos.ysbuffers.find("avg")->second;
            //float* avg = (float*)avgbuffer->data();
            ////HJBuffer::Ptr maxbuffer = plotInfos.ysbuffers.find("max")->second;
            ////float* max = (float*)maxbuffer->data();
            ////HJBuffer::Ptr minbuffer = plotInfos.ysbuffers.find("min")->second;
            ////float* min = (float*)minbuffer->data();
            //HJBuffer::Ptr upperbuffer = plotInfos.ysbuffers.find("upper")->second;
            //float* upper = (float*)upperbuffer->data();
            //HJBuffer::Ptr lowerbuffer = plotInfos.ysbuffers.find("lower")->second;
            //float* lower = (float*)lowerbuffer->data();

            ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, plotInfos.xsMin, plotInfos.xsMax, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.2 * ysMax, ImGuiCond_Always);
            if (show_fills) {
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                ImPlot::PlotShaded("queue shaded", xs, ys, plotInfos.size, m_fillRefValue);
                ImPlot::PopStyleVar();
            }
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f, 0.95f, 0.2f, 1.0f));
            ImPlot::PlotLine("queue", xs, ys, plotInfos.size);
            ImPlot::PopStyleColor();
            ImPlot::PlotLine("100kbps", xs, kbps, plotInfos.size);
            ImPlot::PlotLine("fps", xs, fps, plotInfos.size);
            ImPlot::PlotLine("delay", xs, delay, plotInfos.size);
            ImPlot::PlotLine("net 10Mbps", xs, netkbps, plotInfos.size);

            //ImPlot::PlotLine("duration", xs, max, plotInfos.size);
            //ImPlot::PlotLine("duration", xs, min, plotInfos.size);
            //ImPlot::PlotLine("upper", xs, upper, plotInfos.size);
            //ImPlot::PlotLine("lower", xs, lower, plotInfos.size);

            //
            ImPlot::EndPlot();
        }
    }
#endif
    return HJ_OK;
}

int HJPlayerView::drawFrame(const HJRectf& rect)
{
    int res = HJ_OK;
    do
    {
        const ImGuiIO& io = ImGui::GetIO();
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;
        //
        ImGui::SetNextWindowPos({ rect.x, rect.y });
        ImGui::SetNextWindowSize({ rect.w, rect.h });
        
        static bool fwiewRuning = true;
        if (ImGui::Begin(HJAnsiToUtf8(m_name + "fview").c_str(), &fwiewRuning, window_flags))
        {
            //if (!m_imgView) {
            //    m_imgView = std::make_shared<HJImageView>();
            //    std::string imgName = HJContext::Instance().getResDir() + "/bg/103047.jpg";
            //    res = m_imgView->init(imgName);
            //}
            //m_imgView->draw();

            if (!m_mvfView) {
                m_mvfView = std::make_shared<HJFrameView>();
                res = m_mvfView->init();
            }
            HJ_AUTO_LOCK(m_mutex);
            if (m_mvf) {
                m_mvfView->draw(m_mvf);
            }

            //
            //drawPusherStat(rect);
            drawMuxerStat(rect);
        }
        ImGui::End();
    } while (false);

    return res;
}

int HJPlayerView::drawStatusBar(const HJRectf& rect)
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
            //ImGui::Text(ICON_FA_PAINT_BRUSH"Input Url : "); ImGui::SameLine();
            char filename[1024];
            strcpy(filename, HJAnsiToUtf8(m_mediaUrl).c_str());
            if (ImGui::InputText(u8"Input Url ", filename, HJ_ARRAY_ELEMS(filename))) {
                m_mediaUrl = HJ2SSTR(filename);
            }
            ImGui::SameLine();
            if (!m_btnFileDialog) {
                std::string iconName = "icon_open_file.png";
                std::string tips = "open file";
                m_btnFileDialog = std::make_shared<HJImageButton>();
                res = m_btnFileDialog->init(iconName, tips, { HJView::K_ICON_SIZE_W_DEFAULT, HJView::K_ICON_SIZE_H_DEFAULT });
                if (HJ_OK != res) {
                    HJLoge("error, init file dialog failed");
                    return res;
                }
                m_btnFileDialog->onBtnClick = [&]() {
                    if (!m_viewFileDialog) {
                        m_viewFileDialog = std::make_shared<HJFileDialogView>();
                        res = m_viewFileDialog->init("Open a media file", HJFileDialogView::KEY_MEDIA_FILTER, true, HJContext::Instance().getMediaDir());
                        if (HJ_OK != res) {
                            HJLoge("error, init file dialog view failed");
                        }
                    }
                };
            }
            if (m_btnFileDialog) {
                m_btnFileDialog->draw();
            }
            if (m_viewFileDialog) {
                res = m_viewFileDialog->draw([&](const std::vector<std::filesystem::path>& files) {
                    if (files.size() > 0) {
                        m_mediaUrl = files[0].generic_string();
                        strcpy(filename, HJAnsiToUtf8(m_mediaUrl).c_str());
                    }
                    });
                if (HJ_OK != res) {
                    HJLoge("error, draw file dialog view failed");
                    return res;
                }
                if (m_viewFileDialog->isDone()) {
                    m_viewFileDialog = nullptr;
                }
            }
            ImGui::SameLine();
            ImGui::Checkbox("async", &m_isAsyncUrl);
            
            //
            ImGui::SameLine();
            ImGui::Checkbox("Show Pusher", &m_showPusher);
            if (m_showPusher)
            {
                ImGui::SameLine(); //ImGui::SameLine(0.0f, 10 * style.ItemSpacing.x);
                //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10 * style.ItemSpacing.x);
                //ImGui::Text("Push  Url : "); ImGui::SameLine();
                char pushUrl[1024];
                strcpy(pushUrl, HJAnsiToUtf8(m_pushUrl).c_str());
                if (ImGui::InputText(u8"Push Url ", pushUrl, HJ_ARRAY_ELEMS(pushUrl))) {
                    m_pushUrl = HJ2SSTR(pushUrl);
                }
                //
                //ImGui::SameLine();
                //if (!m_btnPush) {
                //    m_btnPush = std::make_shared<HJImageButton>();
                //    res = m_btnPush->init("ic_run.png", u8"start push", { HJView::K_ICON_SIZE_W_DEFAULT, HJView::K_ICON_SIZE_H_DEFAULT }, [&]() {
                //        //HJMainExecutorAsync([=]() {
                //        //    onStreamAnalyzer();
                //        //});
                //        });
                //    if (HJ_OK != res) {
                //        HJLoge("error, init push button failed");
                //        return res;
                //    }
                //}
                //if (m_btnPush) {
                //    m_btnPush->draw();
                //}
            }
            //
            ImGui::SameLine();
            ImGui::Checkbox("Media Infos", &m_showMInfoWindow);
            ImGui::SameLine();
            ImGui::Checkbox("Save Thumbs", &m_thumbnail);
            if (m_thumbnail) {
                ImGui::SameLine();
                char thumbUrl[1024];
                strcpy(thumbUrl, HJAnsiToUtf8(m_thumbUrl).c_str());
                if (ImGui::InputText(u8"Thumb Url", thumbUrl, HJ_ARRAY_ELEMS(thumbUrl))) {
                    m_thumbUrl = HJ2SSTR(thumbUrl);
                }
            }
            //
            ImGui::SameLine();
            if (!m_btnNetBlock)
            {
                m_btnNetBlock = std::make_shared<HJComplexImageButton>();
                res = m_btnNetBlock->init({ { u8"net running", "ic_running.png" }, {u8"block", "ic_block.png"} }, { HJView::K_ICON_SIZE_W_DEFAULT, HJView::K_ICON_SIZE_H_DEFAULT }, [&](const std::string tips) {
                    HJMainExecutorAsync([=]() {
                            onNetBlock(tips);
                        });
                    });
                if (HJ_OK != res) {
                    HJLoge("error, init run button failed");
                    return res;
                }
            }
            if (m_btnNetBlock) {
                m_btnNetBlock->draw();
            }
            //
            ImGui::SameLine();
            if (!m_btnDataBlock)
            {
                m_btnDataBlock = std::make_shared<HJComplexImageButton>();
                res = m_btnDataBlock->init({ { u8"data running", "ic_running.png" }, {u8"block", "ic_block.png"} }, { HJView::K_ICON_SIZE_W_DEFAULT, HJView::K_ICON_SIZE_H_DEFAULT }, [&](const std::string tips) {
                    HJMainExecutorAsync([=]() {
                            onDataBlock(tips);
                        });
                    });
                if (HJ_OK != res) {
                    HJLoge("error, init run button failed");
                    return res;
                }
            }
            if (m_btnDataBlock) {
                m_btnDataBlock->draw();
            }
            //
            ImGui::SameLine();
            if (!m_comboNetBitrate) 
            {
                static const std::vector<std::string> g_bitrateItems = { "0", "200", "300", "500", "700", "1000", "1200", "1500", "2000", "3000", "4000", "10000" };
                m_comboNetBitrate = std::make_shared<HJCombo>();
                res = m_comboNetBitrate->init(g_bitrateItems, 2, [&](const int idx) {
                    HJMainExecutorAsync([=]() {
                        std::string item = g_bitrateItems[idx];
                        float speed = std::stof(item) * 1000;
                        onNetBitrate(speed);
                        });
                    });
                if (HJ_OK != res) {
                    HJLoge("error, init speed combo failed");
                    return res;
                }
            }
            if (m_comboNetBitrate) {
                m_comboNetBitrate->draw();
            }

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

int HJPlayerView::drawPlayerBar(const HJRectf& rect)
{
    int res = HJ_OK;
    //HJFLogi("draw entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
    do
    {
        const ImGuiIO& io = ImGui::GetIO();
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        //
        ImGui::SetNextWindowPos({ rect.x + 0.25f * rect.w, rect.h - 3.f * rect.y });
        ImGui::SetNextWindowSize({ 0.5f * rect.w, 2.5f * rect.y });
        ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background

        static bool payerBarRuning = true;
        if (ImGui::Begin(HJAnsiToUtf8(m_name + "playerbar").c_str(), &payerBarRuning, window_flags))
        {
            auto minfo = getMediaInfo();
            HJProgressInfo::Ptr pinfo = nullptr;
            if (!m_progressView) 
            {
                m_progressView = std::make_shared<HJProgressView>();
                res = m_progressView->init();
                if (HJ_OK != res) {
                    break;
                }
            }
            if (m_progressView)
            {
                int64_t value = m_curPos;
                int64_t value_min = 0;
                int64_t value_max = 0;

                if (minfo) {
                    value_max = minfo->getDuration();
                }
                pinfo = getProgressInfo();
                if (pinfo) {
                    value = pinfo->getPos();
                }
                if (m_progressView->draw(&value, value_min, value_max)) {
                    m_curPos = value;
                    HJMainExecutorAsync([=]() {
                        onSeek(value);
                    });
                }
            }
            ImGui::NewLine();
            ImGui::SameLine(0.0f, 7 * style.ItemSpacing.x);
            if (!m_btnPrepare) 
            {
                m_btnPrepare = std::make_shared<HJImageButton>();
                res = m_btnPrepare->init("ic_reset.png", u8"prepare", { HJView::K_ICON_SIZE_W_DEFAULT, HJView::K_ICON_SIZE_H_DEFAULT }, [&]() {
                    HJMainExecutorAsync([=]() {
                        if (m_btnRun) {
                            m_btnRun->reset();
                        }
                        onClickPrepare();
                        //onTestHLS();
                        //onTestByteBuffer();
                    });
                    //onTest();
                });
                if (HJ_OK != res) {
                    HJLoge("error, init run button failed");
                    return res;
                }
            }
            if (m_btnPrepare) {
                m_btnPrepare->draw();
            }
            ImGui::SameLine();
            if (!m_btnRun) 
            {
                m_btnRun = std::make_shared<HJComplexImageButton>();
                res = m_btnRun->init({ { u8"play", "ic_play2.png" }, {u8"pause", "icon_pause.png"} }, { HJView::K_ICON_SIZE_W_DEFAULT, HJView::K_ICON_SIZE_H_DEFAULT }, [&](const std::string tips) {
                    HJMainExecutorAsync([=]() {
                        onClickPlay(tips);
//                        onTest();
                    });
//                    onTest1();
                });
                if (HJ_OK != res) {
                    HJLoge("error, init run button failed");
                    return res;
                }
            }
            if (m_btnRun) {
                m_btnRun->draw();
            }
            ImGui::SameLine();
            if (!m_comboSpeed) 
            {
                static const std::vector<std::string> g_speedItems = { "0.5", "0.7", "0.8", "0.9", "1.0", "1.25", "1.5", "2.0", "3.0", "5.0", "8.0", "10.0" };
                m_comboSpeed = std::make_shared<HJCombo>();
                res = m_comboSpeed->init(g_speedItems, 2, [&](const int idx) {
                    HJMainExecutorAsync([=]() {
                        std::string item = g_speedItems[idx];
                        if (m_player) {
                            float speed = std::stof(item);
                            m_player->speed(speed);
                        }
                    });
                });
                if (HJ_OK != res) {
                    HJLoge("error, init speed combo failed");
                    return res;
                }
            }
            if (m_comboSpeed) {
                m_comboSpeed->draw();
            }
            ImGui::SameLine();
            if (!m_btnMute)
            {
                m_btnMute = std::make_shared<HJComplexImageButton>();
                res = m_btnMute->init({ { u8"speaker", "ic_speaker.png" }, {u8"mute", "ic_mute.png"} }, { HJView::K_ICON_SIZE_W_DEFAULT, HJView::K_ICON_SIZE_H_DEFAULT }, [&](const std::string tips) {
                    HJMainExecutorAsync([=]() {
                            onClickMute(tips);
                        });
                    });
                if (HJ_OK != res) {
                    HJLoge("error, init mute button failed");
                    return res;
                }
            }
            if (m_btnMute) {
                m_btnMute->draw();
            }
            ImGui::SameLine();
            try
            {
                if (!m_volumeView) {
                    m_volumeView = std::make_shared<HJSlider>("volume", 0.0f, 1.0f, 1.0f, [&](const float volume) {
                        HJMainExecutorAsync([=]() {
                            onClickVolume(volume);
                        }); 
                    });
                }
                if (m_volumeView) {
                    m_volumeView->draw();
                }
            }
            catch (const HJException& e)
            {
                HJFLoge("error, create slider failed, detail:{}", e.what());
            }

            //
            if (minfo)
            {
                int64_t duration = minfo->getDuration();
                std::string durStr = HJUtilitys::formatMSToString(duration);
                std::string posStr = "00:00:00:000";
                if (pinfo) {
                    int64_t pos = pinfo->getPos();
                    posStr = HJUtilitys::formatMSToString(pos);
                }
                //std::string outStr = HJFMT("{}/{}", posStr, durStr); // posStr + "/" + durStr;

                float spacing_w = ImGui::GetWindowSize().x - ImGui::GetFontSize() * durStr.size(); //10 * style.ItemSpacing.x;
                ImGui::SameLine(spacing_w, 0.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s / %s", posStr.c_str(), durStr.c_str());
            }

            //ImGui::SameLine(0.0f, 10 * style.ItemSpacing.x);
            //if (!m_loadingView) {
            //    m_loadingView = std::make_shared<HJLoadingView>();
            //    res = m_loadingView->init();
            //    if (HJ_OK != res) {
            //        break;
            //    }
            //}
            //if (m_loadingView) {
            //    m_loadingView->draw();
            //}
        }
        ImGui::End();
    } while (false);

    return res;
}

int HJPlayerView::drawMediaInfo(const HJRectf& rect)
{
    auto minfo = getMediaInfo();
    if (!minfo || !m_showMInfoWindow) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    //HJFLogi("draw entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
    do
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::SetNextWindowPos({ rect.x + 0.5f * rect.w, 3.0f * rect.y});
        ImGui::SetNextWindowBgAlpha(0.3f);

        if (ImGui::Begin(HJAnsiToUtf8("media infos").c_str(), &m_showMInfoWindow, window_flags))
        {
            const auto vinfo = minfo->getVideoInfo();
            if (vinfo) {
                std::string infos = HJFMT("[Video:{} width:{}, height:{}, frame rate:{}]  ", AVCodecIDToString((AVCodecID)vinfo->m_codecID), vinfo->m_width, vinfo->m_height, vinfo->m_frameRate);
                ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.92f, 1.0f), infos.c_str());
            }
            HJAudioInfo::Ptr ainfo = minfo->getAudioInfo();
            if (ainfo) {
                std::string infos = HJFMT("[Audio:{} channels:{}, sample rate:{}, byte per sample:{}] ", AVCodecIDToString((AVCodecID)ainfo->m_codecID), ainfo->m_channels, ainfo->m_samplesRate, ainfo->m_bytesPerSample);
                ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.92f, 1.0f), infos.c_str());
            }
            std::string infos = HJFMT("Total: duration:{}", minfo->getDuration());
            ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.92f, 1.0f), infos.c_str());

            //
            HJ_AUTO_LOCK(m_mutex);
            for (const auto& minfo : m_minfos)
            {
                if (HJ_ISTYPE(*minfo, HJVideoInfo)) {
                    ImGui::Separator();
                    auto vinfo = std::dynamic_pointer_cast<HJVideoInfo>(minfo);
                    std::string infos = HJFMT("[Video:{} width:{}, height:{}, frame rate:{}]  ", AVCodecIDToString((AVCodecID)vinfo->m_codecID), vinfo->m_width, vinfo->m_height, vinfo->m_frameRate);
                    ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.92f, 1.0f), infos.c_str());
                }
                else if (HJ_ISTYPE(*minfo, HJAudioInfo)) {
                    ImGui::Separator();
                    auto ainfo = std::dynamic_pointer_cast<HJAudioInfo>(minfo);
                    std::string infos = HJFMT("[Audio:{} channels:{}, sample rate:{}, byte per sample:{}] ", AVCodecIDToString((AVCodecID)ainfo->m_codecID), ainfo->m_channels, ainfo->m_samplesRate, ainfo->m_bytesPerSample);
                    ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.92f, 1.0f), infos.c_str());
                }
            }
        }
        ImGui::End();
    } while (false);

    return res;
}

int HJPlayerView::draw(const HJRectf& rect)
{
    int res = HJ_OK;
    //HJFLogi("draw entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
    do
    {
        res = drawFrame(rect);
        if (HJ_OK != res) {
            break;
        }
        res = drawStatusBar(rect);
        if (HJ_OK != res) {
            break;
        }
        res = drawPlayerBar(rect);
        if (HJ_OK != res) {
            break;
        }
        res = drawMediaInfo(rect);
        if (HJ_OK != res) {
            break;
        }
    } while (false);

    return res;
}

void HJPlayerView::onClickPrepare()
{
    if (m_mediaUrl.empty()) {
        return;
    }
    HJFLogi("entry");
    int res = HJ_OK;
    HJTicker ticker;
    do
    {
        done();
        
        HJFLogi("create player");
        //
        HJMediaUrlVector mediaUrls;
        //HJMediaUrl::Ptr url0 = std::make_shared<HJMediaUrl>("E:/js/1718781990_2.ts");
        //HJMediaUrl::Ptr url1 = std::make_shared<HJMediaUrl>("E:/movies/720x1280.mp4");
        //HJMediaUrl::Ptr url2 = std::make_shared<HJMediaUrl>("E:/movies/qiyiboshi2.mp4");
        //url->setLoopCnt(2);
        //mediaUrls = { url0, url1, url2 };
//        HJMediaUrl::Ptr url0 = std::make_shared<HJMediaUrl>("E:/js/_LC_AL2_non_27013848617200603950012820_OX.flv");
        //HJMediaUrl::Ptr url0 = std::make_shared<HJMediaUrl>("E:/js/587102.crdownload.flv");
        std::string url = m_mediaUrl;
        if (m_isAsyncUrl) {
            url = "exasync:" + url;
        }
        //auto rid = HJ2STR(HJUtilitys::hash(url));
        //url = HJLocalServer::getInstance()->getPlayUrl(rid, url);
        //
        HJMediaUrl::Ptr url0 = std::make_shared<HJMediaUrl>(url);
        url0->setTimeout(m_timeout);
//        url0->setLoopCnt(100);
        //url0->setUseFast(false);
        //url0->setDisableMFlag(HJMEDIA_TYPE_VIDEO);
        mediaUrls = { url0 };

        //{
        //    auto testUrl = std::make_shared<HJMediaUrl>("https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8");
        //    HJM3U8Parser::Ptr parser = std::make_shared<HJM3U8Parser>();
        //    parser->init(testUrl);

        //    auto segGroups = parser->getSegmentGroups();
        //    auto totalDuration = parser->getTotalDurationMs();
        //    HJFLogi("totalDuration:{}", totalDuration);
        //}
        //
        m_player = std::make_shared<HJMediaPlayer>();
        m_player->setMediaFrameListener([&](const HJMediaFrame::Ptr mvf) -> int {
            if (mvf->isVideo()) {
                //HJFLogi("recv media frame:" + mvf->formatInfo());
                HJ_AUTO_LOCK(m_mutex);
                m_mvf = mvf;
                if (m_thumbnail) {
                    onWriteThumb(m_mvf);
                }
            }
            return HJ_OK;
        });
        m_player->setSourceFrameListener([&](const HJMediaFrame::Ptr mavf) -> int {
            if (m_showMuxer) {
                onMuxer(mavf);
            }
            if (m_showPusher) {
                onPusher2(mavf);
            }
            return HJ_OK;
        });
        res = m_player->init(mediaUrls, [&](const HJNotification::Ptr ntf) -> int {
            //HJLogi("notify begin, id:" + HJNotifyIDToString((HJNotifyID)ntf->getID()) + ", value:" + HJ2STR(ntf->getVal()) + ", msg:" + ntf->getMsg());
            switch (ntf->getID())
            {
            case HJNotify_Prepared:
            {
                auto minfo = ntf->getValue<HJMediaInfo::Ptr>(HJMediaInfo::KEY_WORLDS);
                if (minfo) {
                    setMediaInfo(minfo);
                }
                HJMainExecutorAsync([=]() {
                    onStart();
                    });
                break;
            }
            case HJNotify_NeedWindow:
            {
                onSetWindow();
                break;
            }
            case HJNotify_ProgressStatus:
            {
                auto progInfo = ntf->getValue<HJProgressInfo::Ptr>(HJProgressInfo::KEY_WORLDS);
                if (progInfo) {
                    HJ_AUTO_LOCK(m_mutex);
                    m_progInfo = progInfo;
                }
                break;
            }
            case HJNotify_Already:
            {
                onClickPlay("play");
                break;
            }
            case HJNotify_DemuxEnd:
            {
                break;
            }
            case HJNotify_Complete:
            {
                onCompleted();
                break;
            }
            case HJNotify_Error:
            {
                int ret = onProcessError(ntf->getVal());
                if (HJ_OK != ret) {
                    HJFLoge("error, on process error failed, m_tryCnt:{}", m_tryCnt);
                    break;
                }
                //HJMainExecutorAsync([=]() {
                //    onClickPrepare();
                //});
                break;
            }
            case HJNotify_MediaChanged:
            {
                auto minfo = ntf->getValue<HJStreamInfo::Ptr>(HJStreamInfo::KEY_WORLDS);
                if (minfo) {
                    HJ_AUTO_LOCK(m_mutex);
                    m_minfos.emplace_back(minfo);
                }
                break;
            }
            default: {
                break;
            }
            }
            return HJ_OK;
        });
    } while (false);
    HJFLogi("end, time:{}", ticker.elapsedTime());

    return;
}

void HJPlayerView::onClickPlay(const std::string& tips)
{
    if (!m_player || !m_player->isReady()) {
        return;
    }
    if ("play" == tips) {
        m_player->play();
    }
    else {
        m_player->pause();
    }
    //HJMainExecutor()->async([&] {
    //    onClickPrepare();
    //}, HJUtilitys::random(2000, 5000));

    return;
}

void HJPlayerView::onStart()
{
    if (!m_player) {
        return;
    }
    m_player->start();

    return;
}

void HJPlayerView::onSetWindow()
{
    if (!m_player) {
        return;
    }
    m_player->setWindow((HJHND)m_player.get());

    return;
}

void HJPlayerView::onSeek(const int64_t pos)
{
    if (!m_player) {
        return;
    }
    HJFLogi("entry, pos:{}", pos);
    m_player->seek(pos);

    return;
}

void HJPlayerView::onCompleted()
{
    HJFLogi("entry");
    //HJMainExecutor()->async([=]() {
    //    onClickPrepare();
    //}, 1000);
}

void HJPlayerView::onWriteThumb(const HJMediaFrame::Ptr& frame)
{
    int res = HJ_OK;
    //if (!m_imageWriter) {
    //    m_imageWriter = std::make_shared<HJImageWriter>();
    //    res = m_imageWriter->initWithPNG(180, 320);
    //    if (HJ_OK != res) {
    //        HJFLoge("error, create image writer failed:{}", res);
    //        return;
    //    }
    //    HJFileUtil::delete_file(m_thumbUrl);
    //    if (!HJFileUtil::is_dir(m_thumbUrl)) {
    //        HJFileUtil::makeDir(m_thumbUrl);
    //    }
    //}
    //if (m_mvf && m_imageWriter) {
    //    std::string thumbUrl = HJFMT("{}/{}_{}x{}.png", m_thumbUrl, m_imageIdx, m_mvf->getVideoInfo()->m_width, m_mvf->getVideoInfo()->m_height);
    //    res = m_imageWriter->writeFrame(m_mvf, thumbUrl);
    //    m_imageIdx++;
    //}
}

int HJPlayerView::onProcessError(const int err)
{
    if (m_tryCnt >= HJ_TRY_PLAY_CNT_MAX) {
        return HJErrFatal;
    }
    switch (err)
    {
    case HJErrTimeOut:
        m_timeout = m_timeout * 3 / 2;
        break;
    default:
        break;
    }
    m_tryCnt++;

    return HJ_OK;
}

int HJPlayerView::onMuxer(const HJMediaFrame::Ptr& frame)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do
    {
        //HJFLogi("source frame:{}", frame->formatInfo());
        if (!m_muxer) {
            std::string url = HJUtilitys::concatenatePath(m_muxerUrl, HJUtilitys::concateString(HJUtilitys::getTimeToString(), ".mp4"));
            //m_muxer = std::make_shared<HJFFMuxer>();
            //auto outUrl = std::make_shared<HJMediaUrl>(url);
            //m_minfo->setMediaUrl(outUrl);
            //res = m_muxer->init(m_minfo);
            m_muxer = HJCreates<HJFFMuxer>();
            res = m_muxer->init(url);
            if (HJ_OK != res) {
                break;
            }
            //m_muxer->setIsLive(true);
            m_muxer->setTimestampZero(true);
        }
        res = m_muxer->writeFrame(frame);
        if (HJ_OK != res) {
            break;
        }
    } while (false);
    return res;
}

int HJPlayerView::onPusher(const HJMediaFrame::Ptr& frame)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do
    {
        //HJFLogi("source frame:{}", frame->formatInfo());
        //if (!m_pusher)
        //{
        //    m_pusher = std::make_shared<HJJPusher>();
        //    auto outUrl = std::make_shared<HJMediaUrl>(m_pushUrl);
        //    m_minfo->setMediaUrl(outUrl);
        //    res = m_pusher->init(m_minfo);
        //    if (HJ_OK != res) {
        //        break;
        //    }
        //}
        //res = m_pusher->writeFrame(frame);
        //if (HJ_OK != res) {
        //    break;
        //}
    } while (false);
    return res;
}

int HJPlayerView::onPusher2(const HJMediaFrame::Ptr& frame)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do
    {
        //HJFLogi("source frame:{}", frame->formatInfo());
        if (!m_rtmpMuxer)
        {
            m_rtmpMuxer = HJCreates<HJRTMPMuxer>([&](const HJNotification::Ptr ntfy) -> int {
                if (!ntfy) {
                    return HJ_OK;
                }
                HJLogi("notify:{}", ntfy->getMsg());

                return HJ_OK;
            });

            //auto outUrl = std::make_shared<HJMediaUrl>(m_pushUrl);
            //outUrl->setOutUrl("E:/MMGroup/dumps/test.flv");
            //m_minfo->setMediaUrl(outUrl);
            //res = m_rtmpMuxer->init(m_minfo);
            int mediaTypes = HJMEDIA_TYPE_AV;
            std::string localUrl = "E:/MMGroup/dumps/test.flv";
            auto options = std::make_shared<HJOptions>();
            //(*options)[HJRTMPUtils::STORE_KEY_LOCALURL] = localUrl;
            //(*options)[HJRTMPUtils::STORE_KEY_SOCKET_BUF_SIZE] = (int)5 * 1024 * 1024;
            //(*options)[HJRTMPUtils::STORE_KEY_DROP_ENABLE] = bool(false);
            //HJRTMPUtils::STORE_KEY_RETRY_TIME_LIMITED
            //HJRTMPUtils::STORE_KEY_DROP_THRESHOLD
            //HJRTMPUtils::STORE_KEY_DROP_PFRAME_THRESHOLD;
            //HJRTMPUtils::STORE_KEY_DROP_IFRAME_THRESHOLD;
            (*options)[HJRTMPUtils::STORE_KEY_LOWBR_TIMEOUT_ENABLE] = true;
            (*options)[HJRTMPUtils::STORE_KEY_LOWBR_TIMEOUT_LIMITED] = (int)60*1000;
            (*options)[HJRTMPUtils::STORE_KEY_LOWBR_LIMITED] = (int)600;
            res = m_rtmpMuxer->init(m_pushUrl, mediaTypes, options);
            if (HJ_OK != res) {
                break;
            }
        }
        frame->setExtraTS(HJCurrentSteadyMS());

        //auto seiFrame = makeSEINals();
        auto seiFrame = makePacketSEINals();
        (*frame)[HJMediaFrame::STORE_KEY_SEIINFO] = seiFrame;

        res = m_rtmpMuxer->writeFrame(frame);
        if (HJ_OK != res) {
            break;
        }
    } while (false);
    return res;
}

void HJPlayerView::onClickMute(const std::string& tips)
{
    if (!m_player || !m_player->isReady()) {
        return;
    }
    if ("speaker" == tips) {
        m_player->setVolume(0.0f);
    }
    else {
        m_player->setVolume(1.0f);
    }
}

void HJPlayerView::onClickVolume(const float volume)
{
    if (!m_player || !m_player->isReady()) {
        return;
    }
    HJFLogi("set volume:{}", volume);
    m_player->setVolume(volume);
}

void HJPlayerView::onTest()
{
    //HJLogi("onTest -- entry");
    //HJDefaultExecutor()->async([&]() {
    //    HJLogi("onTest --  async entry");
    //    int64_t pos = HJUtilitys::random(0, m_minfo->getDuration());
    //    onSeek(pos);
    //    
    //    onTest();
    //}, HJUtilitys::random(500, 2000));
    //m_sem.notify();
    
    //std::thread exe([]() {
    //    do
    //    {
    //        HJLogi("thread entry");
    //        auto executor = HJCreateExecutor(HJExecutorGlobalName("test"));
    //        executor->sync([&]() {
    //            HJLogi("thread sync func");
    //            });
    //        executor = nullptr;
    //        //std::thread t([]() {
    //        //    HJLogi("thread sync func");
    //        //    });
    //        //t.join();

    //        //std::uniform_int_distribution<int> delay_distribution(500, 1500);
    //        std::this_thread::sleep_for(std::chrono::milliseconds(HJUtilitys::random(500, 1500)));

    //        HJLogi("thread end");
    //    } while (true);
    //});

    //do
    //{
    //    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //} while (true);
    //HJLogi("onTest end");

    return;
}

void HJPlayerView::onTest1()
{
    HJLogi("onTest1 --  entry");
//    m_sem.wait();
    //m_sem.waitBFor(HJSeconds(10));
//    HJLogi("onTest1 --  end");
    //HJDefaultExecutor()->async([&]() {
    //    HJLogi("onTest1  -- async entry");
    //    });
}

void HJPlayerView::onTestHLS()
{
    HJMediaUrl::Ptr url = std::make_shared<HJMediaUrl>("https://live-replay-5.test.huajiao.com/psegments/z1.huajiao-live.HJ_0_qiniu_5_huajiao-live__h265_45752749_1730776617184_3550_T/1730776662-1730776884.m3u8");
    //m_hlsParser = std::make_shared<HJHLSParser>();
    //int ret = m_hlsParser->init(url);

    return;
}

void HJPlayerView::onTestByteBuffer()
{
    //HJByteBuffer::Ptr buffer = std::make_shared<HJByteBuffer>();
    //buffer->write<uint8_t>(9);
    //buffer->write<uint16_t>(11);
    //buffer->write24(12);
    //buffer->write<uint32_t>(13);
    //buffer->write<uint64_t>(14);
    //
    //auto data = buffer->data();
    //auto size = buffer->size();

    return;
}

void HJPlayerView::onNetBlock(const std::string& tips)
{
    //if (!m_pusher) {
    //    return;
    //}
    //if(tips == "block") {
    //    m_pusher->setNetBlock(false);
    //} else {
    //    m_pusher->setNetBlock(true);
    //}

    if (!m_rtmpMuxer) {
        return;
    }
    if(tips == "block") {
        m_rtmpMuxer->setNetBlock(false);
    } else {
        m_rtmpMuxer->setNetBlock(true);
    }
    return;
}
void HJPlayerView::onDataBlock(const std::string& tips)
{
    //if (!m_pusher) {
    //    return;
    //}
    //if (tips == "block") {
    //    m_pusher->setDataBlock(false);
    //} else {
    //    m_pusher->setDataBlock(true);
    //}
}

void HJPlayerView::onNetBitrate(int bitrate)
{
    if (!m_rtmpMuxer) {
        return;
    }
    m_rtmpMuxer->setNetSimulater(bitrate);
    return;
}

HJSEINals::Ptr HJPlayerView::makeSEINals()
{
    HJSEINals::Ptr seiNals = HJCreates<HJSEINals>();

    std::string set0 = HJMakeGlobalName("00000001_huajiao_hello");
    HJSEIData seiData;
    seiData.uuid = HJUUIDTools::HJ_DERIVE_UUID0.toString();
    seiData.data = HJUtilitys::makeBytes(set0);

    bool isH265 = true;
    auto out_nal = HJSEIWrapper::makeSEINal({ seiData }, isH265, HJSEIWrapper::HJNALFormat::AVCC);
    seiNals->addData(out_nal);

    return seiNals;
}

HJSEINals::Ptr HJPlayerView::makePacketSEINals()
{
    HJDataFuse::Ptr dataFuse = HJCreates<HJDataFuse>();

    std::string msg = "TOOL CFG: width:720 height:1280 fps:25:1 timebase:1:25 vfr:0 vui:1:1:0:5:1:1:1:5:format:1 preset:7 tune:meetingCamera level:50 repeat-header:1 ref:3 long-term:0 open-gop:0 keyint:50:5 rc:1 cqp:32 cbr/abr:1548 crf:0:0:0 vbv-max-bitrate:2012:1 vbv-buf-size:2012:4 vbv-buf-init:0.9 max-frame-ratio:100 ip-factor:2 rc-peakratetol:5 qp:40:22:35:22 aq:5:1 wpp:1 pool-threads:2:64 svc:0 fine-tune:-1 roi:1:1 TOOL CFG: qpInitMin:22 qpInitMax:35 MaxRatePeriod:1 MaxBufPeriod:4 HAD:1 FDM:1 RQT:1 TransformSkip:0 SAO:1 TMVPMode:1 SignBitHidingFlag:1 MvStart:4 BiRefNum:0 FastSkipSub:1 EstimateCostSkipSub0:0 EstimateCostSkipSub1:0 EstimateCostSkipSub2:0 SkipCurFromSplit0:1 SkipCurFromSplit1:2 SkipCurFromSplit2:3 SubDiffThresh0:18 SubDiffThresh1:10 SubDiffThresh2:6 FastMergeSkip:1 SkipUV:1 RefineSkipUV:0 MergeSkipTh0:30000 MergeSkipTh1:30000 MergeSkipTh2:40 MergeSkipTh3:40 MergeSkipTh:0 MergeSkipDepth:0 MergeSkipEstopTh0:350 MergeSkipEstopTh1:250 MergeSkipEstopTh2:150 MergeSkipEstopTh3:100 CbfSkipIntra:1 neiIntraSkipIntra:6 SkipFatherIntra:0 SkipIntraFromRDCost:3 StopIntraFromDist0:11 StopIntraFromDist1:12 StopIntraFromDist2:14 StopIntraFromDist3:16 TopdownContentTh0:32 TopdownContentTh1:32 TopdownContentTh2:32 TopdownContentTh3:32 TopdownContentTh4:32 BottomUpContentTh0:0 BottomUpContentTh1:14 BottomUpContentTh2:12 BottomUpContentTh3:0 BottomUpContentTh4:0 BottomUpContentTh5:12 madth4merge:128 DepthSkipCur:0 CheckCurFromSubSkip:0 DepthSkipSub:3 StopCurFromNborCost:5 SkipFatherBySubmode:0 SkipL1ByNei:0 CheckCurFromNei:0 EarlySkipAfterSkipMerge:2 sccIntra8distTh:5000 sccIntraNxNdistTh:120 TuEarlyStopPu:18 FastSkipInterRdo:1 IntraCheckInInterFastSkipNxN0:17 IntraCheckInInterFastSkipNxN1:40 IntraCheckInIntraFastSkipNxN:0 IntraCheckInIntraSkipNxN:0 AmpSkipAmp:1 Skip2NAll:0 Skip2NFromSub0:600 Skip2NFromSub1:600 Skip2NFromSub2:650 Skip2NRatio0:450 Skip2NRatio1:450 Skip2NRatio2:450 SkipSubFromSkip2n:1 AdaptMergeNum:5 TuStopPu:18 qp2qstepbetter:610 qp2qstepfast:600 InterCheckMerge:1 skipIntraFromPreAnalysis0:18 skipIntraFromPreAnalysis1:18 skipIntraFromPreAnalysis2:18 DisNxNLevel:3 SubdiffSkipSub0:260 SubdiffSkipSub1:200 SubdiffSkipSub2:120 SubdiffSkipSub:0 SubdiffSkipSubRatio0:10 SubdiffSkipSubRatio1:10 SubdiffSkipSubRatio2:60 SubdiffSkipSubRatio:0 StopSubMaxCosth0:2048 StopSubMaxCosth1:0 StopSubMaxCosth2:0 StopSubMaxCosth3:0 CostSkipSub0:12 CostSkipSub1:6 CostSkipSub2:3 CostSkipSub:1 DistSkipSub0:12 DistSkipSub1:8 DistSkipSub2:4 DistSkipSub3:1 SkipSubNoCbf:1 SaoLambdaRatio0:7 SaoLambdaRatio1:8 SaoLambdaRatio2:8 SaoLambdaRatio3:9 AdaptiveSaoLambda[0]:60 AdaptiveSaoLambda[1]:90 AdaptiveSaoLambda[2]:120 AdaptiveSaoLambda[3]:150 SecModeInInter0:6 SecModeInInter1:0 SecModeInInter2:0 SecModeInInter3:4 SecModeInInter4:1 SecModeInIntra0:6 SecModeInIntra1:0 SecModeInIntra2:0 SecModeInIntra3:3 SecModeInIntra4:1 ChromaModeOptInInter0:0 ChromaModeOptInInter1:0 ChromaModeOptInInter2:0 IntraCheckInInterFastSkipUseNborCu:1 disframesao:1 skipTuSplit:16 FastQuantInter0:95 FastQuantInter1:95 FastQuantInterChroma:20 MeInterpMethod:0 MultiRef:1 BiMultiRef:0 BiMultiRefThr:0 MultiRefTh:0 MultiRefFast2nx2nTh:0 MvStart:4 StopSubFromNborCost:0 StopSubFromNborCost2:80 StopSubFromNborCount:0 imecorrect:1 fmecorrect:1 qmeguidefast:1 unifmeskip:3 fasthme:1 FasterFME:1 AdaptHmeRatio0:0 AdaptHmeRatio1:0 AdaptHmeRatio2:0 skipqme0:0 skipqme1:0 skipqme2:0 skipqme3:23 fastqmenumbaseframetype0:3 fastqmenumbaseframetype1:2 fastqmenumbaseframetype2:2 FastSubQme:12 adaptiveFmeAlg0:0 adaptiveFmeAlg1:0 adaptiveFmeAlg2:0 adaptiveFmeAlg3:0 GpbSkipL1Me:0 BackLongTermReference:1 InterSatd2Sad:2 qCompress:0.6 qCompressFirst: 0.5 CutreeLambdaPos: 2 CutreeLambdaNeg: 100 CutreeLambdaFactor: 0 AdaptPSNRSSIM:40 LHSatdscale:0 PMESearchRange:1 LookAheadHMVP:0 hmvp-thres0 LookAheadNoEarlySkip:1 LHSatdScaleTh:80 LHSatdScaleTh2:180 FreqDomainCostIntra0:-1 FreqDomainCostIntra1:-1 FreqDomainCostIntra2:-1 deblock(tc:beta):0, 0 RCWaitRatio1: 0.5 RCWaitRatio2: 0.2 RCConsist:0 QPstep:4 RoundKeyint:0 VbvPredictorSimple:0.5 VbvPredictorSimpleMin:0.5 VbvPredictorNormal:1 VbvPredictorNormalMin:1 VbvPredictorComplex:1 VbvPredictorComplexMin:1 DpbErrorResilience: 0 aud:0 lookahead-use-sad:1 intra-sad0:1 intra-sad1:1 intra-sad2:0 intra-sad3:0 intra-sad4:0 skip-sao-freq:0 skip-sao-den:0 skip-sao-dist:0 adaptive-quant:0 wpp2CachedTuCoeffRows:0 disablemergemode:0 merge-mode-depth:0 skipCudepth0:0 faster-code-coeff:0 rqt-split-first0 intra-fine-search-opt:0 sao-use-merge-mode0:0 sao-use-merge-mode1:0 sao-use-merge-mode2:0 enable-sameblock:0 sameblock-dqp:255 scclock-th:0 skip-child-by-sameblock:0 skip-no-zero-mv-by-sameblock:0 sameblock-disable-merge:0 tzsearchscc-adaptive-stepsize:0 skip-intra-fine-search-rd-thr2:0 skip-sub-from-father-skip:0 prune-split-from-rdcost:0 check-cur-from-subcost:0 skip-dct:0 list1-fast-ref-cost-thr:0 list1-fast-ref-cost-thr2:0 char-aq-cost-min:0 char-target-qp:0 content-early-stop-thresh0:0 content-early-stop-thresh1:0 content-early-stop-thresh2:0 content-early-stop-thresh3:0 sub-diff-skip-sub-chroma-weight:-1 skip-inter-by-intra:0 enable-contrast-enhancement:0 lowdelay-skip-ref:0 enable-adaptive-me:0 search-thresh:0 search-offset:0 skip-l1-by-sub:0 skip-dct-inter4x4:0 same-ctu-skip-sao:0 skip-l1-by-skip-dir:0 skip-bi-by-father:0 skip-l1-by-father:0 skip-l1-by-nei:0 enable-hmvp:0 check-cur-from-nei:0 skip-sub-from-skip-2n-cudepth:5 lp-single-ref-list-cudepth:-1 ime-round:16 camera-subjective-opt:1 enable-chroma-weight:0 disable-tu-early-stop-for-subjective:0 enable-subjective-loss:0 enable-contrast-enhancement:0 go-down-qp-for-subjective:51 skip-2n-from-sub-qp-for-subjective:51 skip-2n-from-sub-chroma-weight-for-subjective:0 skip-2n-from-sub-chroma-weight4-for-subjective:0 complex-block-thr-factor1-for-subjective:3 complex-block-thr-factor2-for-subjective:0 opt-try-go-up-for-subjective:0 large-qp-large-size-intra-use-4-angle:0 aq-adjust-for-subjective:0 enable-subjective-char-qp:0 hyperfast-tune-subjective:0 hyperfast-tune-detail:0 scenecut-dqp:0 skip-dct-offset0:0 skip-dct-offset1:0 skip-dct-offset2:0 skip-lowres-inter:0 lp-single-ref-list-cu-depth:-1 lp-single-ref-frame-cu-depth:-1 slice-depth0-fast-ref:0 fast-bi-pattern-search:0 skip-intranxn-in-bslice:0 intra-stride-inter-frame:0 topdown-father-skip:0 skip-father-by-submode:0 skip-coarse-intra-pred-mode-by-cost:0 skip-inter64x64:0 skip-sub-from-skip-2n-cudepth:5 inter-search-lowres-mv:0 skip-dct:0 stop-split-by-neibor-skip:0 intra-non-std-dct:0 pool-mask:0 max-merge:3 tu-inter-depth:1 tu-intra-depth:1 sr:256 me:1 pme:0 max-dqp-depth:1 cb-qp-offset:4 cr-qp-offset:4 lookahead-threads:1 vbv-adapt:0 vbv-control:0 roi2-strength:0 roi2-uv-delta:0 roi2-height-up-scale:0.25 roi2-height-down-scale:0.25 roi2-margin-left-scale:0 roi2-margin-right-scale:0";
    auto packets = dataFuse->pack(msg);

    HJSEINals::Ptr seiNals = HJCreates<HJSEINals>();
    for (auto &packet : packets) {
        HJSEIData seiData;
        seiData.uuid = HJUUIDTools::HJ_DERIVE_UUID0.toString();
        seiData.data = packet;

        bool isH265 = true;
        auto out_nal = HJSEIWrapper::makeSEINal({ seiData }, isH265, HJSEIWrapper::HJNALFormat::AVCC);
        seiNals->addData(out_nal);
    }


    //std::optional<int64_t> seq_id = 0;
    //auto play_uuid = HJUUIDTools::derive_uuid(HJPackerManager::HJ_MAIN_UUID, seq_id);
    //std::string uuid_str = "";
    //for (auto it : play_uuid.data) {
    //    uuid_str += HJFMT("0x{:x},", it);
    //}
    //HJFLogi("play_uuid: {}, {}", uuid_str, play_uuid.toString());

    //auto valid =  HJUUIDTools::verify_uuid(HJPackerManager::HJ_MAIN_UUID, play_uuid, seq_id);

    return seiNals;
}


NS_HJ_END
