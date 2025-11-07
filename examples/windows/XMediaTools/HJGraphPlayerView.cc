//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJGraphPlayerView.h"
#include "HJContext.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "HJGlobalSettings.h"
//#include "HJByteBuffer.h"
//#include "IconFontCppHeaders/IconsFontAwesome5.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJGraphPlayerView::HJGraphPlayerView()
{
    m_name = HJMakeGlobalName("Stream Pusher");
}

HJGraphPlayerView::~HJGraphPlayerView()
{
    HJFLogi("~HJGraphPlayerView name:{} entry", m_name);
    HJMainExecutorSync([=]() {
        done();
    });
    HJFLogi("~HJGraphPlayerView name:{} end", m_name);
}

int HJGraphPlayerView::init(const std::string info)
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
        //m_mediaUrl = "http://www.w3school.com.cn/example/html5/mov_bbb.mp4";
        //m_mediaUrl = "exasync:https://live-pull-1.huajiao.com/main/HJ_0_ali_1_main__h265_273151831_1760953590409_3574_O.flv?auth_key=1761040119-0-0-1a394c6054c5f366ca0fc017cbf639bb";
        //m_mediaUrl = "E:/movies/replaym3u8/index.m3u8";
        //m_mediaUrl = "https://live-pull-2.huajiao.com/main/HJ_0_ali_2_main_a_h264_270100274_1732602713389_8267_O.flv?auth_key=1732694308-0-0-d6f19de17979da5df6536153270dc20f";
        //m_mediaUrl = "https://al2-flv.live.huajiao.com/live_huajiao_h265/_LC_AL2_non_h265_SD_26624183417212690010010622_OX.flv";//"E:/js/820827.crdownload.flv";
        m_mediaUrl = "https://live-pull-1.huajiao.com/main/HJ_0_ali_1_main__h265_97986049_1761187545373_8986_O.flv?auth_key=1761275415-0-0-42ee16d4ef6814f2090b0df73433905d";
        //m_mediaUrl = "http://localhost:8080/live/livestream.flv";
#elif defined(HJ_OS_MACOS)
        m_mediaUrl = "/Users/zhiyongleng/works/movies/720x1280.mp4";
#endif
        m_thumbUrl = HJUtilitys::exeDir() + "thumbs/";

        HJFLogi("init url:{}", m_mediaUrl);
    } while (false);

    return res;
}

void HJGraphPlayerView::done()
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

int HJGraphPlayerView::drawMuxerStat(const HJRectf& rect)
{
    if (m_rtmpMuxer)
    {
        HJGraphBPlotInfos plotInfos;
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

int HJGraphPlayerView::drawPusherStat(const HJRectf& rect)
{
#if 0
    if (m_pusher) 
    {
        HJGraphBPlotInfos plotInfos;
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

int HJGraphPlayerView::drawPlayerStat(const HJRectf& rect)
{
    bool show_fills = false;
    const auto plotInfos = getDelayParams();
    ImGui::SameLine();
    if (plotInfos.xsbuffer && ImPlot::BeginPlot("delay params Curve", ImVec2(-1, 0.6 * rect.h)))
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
        HJBuffer::Ptr avgbuffer = plotInfos.ysbuffers.find("avg")->second;
        float* avg = (float*)avgbuffer->data();
        //HJBuffer::Ptr maxbuffer = plotInfos.ysbuffers.find("max")->second;
        //float* max = (float*)maxbuffer->data();
        //HJBuffer::Ptr minbuffer = plotInfos.ysbuffers.find("min")->second;
        //float* min = (float*)minbuffer->data();
        HJBuffer::Ptr upperbuffer = plotInfos.ysbuffers.find("upper")->second;
        float* upper = (float*)upperbuffer->data();
        HJBuffer::Ptr lowerbuffer = plotInfos.ysbuffers.find("lower")->second;
        float* lower = (float*)lowerbuffer->data();

        HJBuffer::Ptr audiorender = plotInfos.ysbuffers.find("ar_data")->second;
        float* ar_data = (float*)audiorender->data();

        ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, plotInfos.xsMin, plotInfos.xsMax, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.2 * ysMax, ImGuiCond_Always);
        if (show_fills) {
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded("duration shaded", xs, ys, plotInfos.size, m_fillRefValue);
            ImPlot::PopStyleVar();
        }
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f, 0.95f, 0.2f, 1.0f));
        ImPlot::PlotLine("duration", xs, ys, plotInfos.size);
        ImPlot::PopStyleColor();
        //ImPlot::PlotLine("avg", xs, avg, plotInfos.size);
        //ImPlot::PlotLine("duration", xs, max, plotInfos.size);
        //ImPlot::PlotLine("duration", xs, min, plotInfos.size);
        ImPlot::PlotLine("upper", xs, upper, plotInfos.size);
        ImPlot::PlotLine("lower", xs, lower, plotInfos.size);

        ImPlot::PlotLine("arender", xs, ar_data, plotInfos.size);

        //
        ImPlot::EndPlot();
    }

    if (plotInfos.xsbuffer && ImPlot::BeginPlot("speed params Curve", ImVec2(-1, 0.4 * rect.h)))
    {
        ImPlot::SetupAxes("ms", "speed");
        //ImPlot::SetupAxesLimits(0, xsMax, 0, 1.5 * ysMax);
        double ysMax = 2.0;//plotInfos.ysMaxs.begin()->second;
        //HJBuffer::Ptr ysbuffer = nullptr;
        //auto it = plotInfos.ysbuffers.find("dura");
        //if (it != plotInfos.ysbuffers.end()) {
        //	ysbuffer = it->second;
        //}
        float* xs = (float*)plotInfos.xsbuffer->data();
        //float* ys = (float*)ysbuffer->data();
        //
        HJBuffer::Ptr speedbuffer = plotInfos.ysbuffers.find("speed")->second;
        float* speed = (float*)speedbuffer->data();
        HJBuffer::Ptr stutterbuffer = plotInfos.ysbuffers.find("stutter")->second;
        float* stutter = (float*)stutterbuffer->data();
        //
        //HJBuffer::Ptr audiodropping = plotInfos.ysbuffers.find("ad_data")->second;
        //float* ad_data = (float*)audiodropping->data();
        //HJBuffer::Ptr videodropping = plotInfos.ysbuffers.find("vd_data")->second;
        //float* vd_data = (float*)videodropping->data();
        //HJBuffer::Ptr audiorender = plotInfos.ysbuffers.find("ar_data")->second;
        //float* ar_data = (float*)audiorender->data();

        ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, plotInfos.xsMin, plotInfos.xsMax, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, ysMax, ImGuiCond_Always);
        if (show_fills) {
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded("speed shaded", xs, speed, plotInfos.size, m_fillRefValue);
            ImPlot::PopStyleVar();
        }
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f, 0.95f, 0.2f, 1.0f));
        ImPlot::PlotLine("speed", xs, speed, plotInfos.size);
        ImPlot::PopStyleColor();
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.95f, 0.0f, 0.2f, 1.0f));
        ImPlot::PlotLine("stutter", xs, stutter, plotInfos.size);
        ImPlot::PopStyleColor();

        //ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.95f, 0.0f, 0.2f, 1.0f));
        //ImPlot::PlotLine("audio drop", xs, ad_data, plotInfos.size);
        //ImPlot::PopStyleColor();
        //ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.95f, 0.0f, 0.2f, 1.0f));
        //ImPlot::PlotLine("vido drop", xs, vd_data, plotInfos.size);
        //ImPlot::PopStyleColor();
        //ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.95f, 0.0f, 0.2f, 1.0f));
        //ImPlot::PlotLine("audio render", xs, ar_data, plotInfos.size);
        //ImPlot::PopStyleColor();

        //
        ImPlot::EndPlot();
    }
    return HJ_OK;
}

int HJGraphPlayerView::drawFrame(const HJRectf& rect)
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
            //drawMuxerStat(rect);
            drawPlayerStat(rect);
        }
        ImGui::End();
    } while (false);

    return res;
}

int HJGraphPlayerView::drawStatusBar(const HJRectf& rect)
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

int HJGraphPlayerView::drawPlayerBar(const HJRectf& rect)
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
                    //HJMainExecutorAsync([=]() {
                    //    std::string item = g_speedItems[idx];
                    //    if (m_player) {
                    //        float speed = std::stof(item);
                    //        m_player->speed(speed);
                    //    }
                    //});
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

int HJGraphPlayerView::drawMediaInfo(const HJRectf& rect)
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

int HJGraphPlayerView::draw(const HJRectf& rect)
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

void HJGraphPlayerView::onCreatePlayer()
{
    if (m_player) {
        return;
    }
    HJFLogi("onCreatePlayer entry, m_mediaUrl:{}, count:{}", m_mediaUrl, m_mediaRepeat++);

    m_player = std::make_shared<HJGraphLivePlayer>();

    auto param = std::make_shared<HJKeyStorage>();
    HJListener playerListener = [&](const HJNotification::Ptr ntf) -> int {
        if (ntf == nullptr) {
            return HJ_OK;
        }

        switch (ntf->getID())
        {
            case HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME:
            {
                auto frame = ntf->getValue<HJMediaFrame::Ptr>("frame");
                if (frame) {
                    HJ_AUTO_LOCK(m_mutex);
                    m_mvf = frame;
                }
                break;
            }
            case HJ_PLUGIN_NOTIFY_AUTODELAY_PARAMS:
            {
                auto delayParams = ntf->getValue<std::shared_ptr<HJParams>>("auto_params");
                if (delayParams) {
                    std::shared_ptr<HJPlayerDelayParams> params = std::make_shared<HJPlayerDelayParams>();
                    if (delayParams->find("stat") != delayParams->end()) {
                        params->stat = std::any_cast<std::string>(delayParams->at("stat"));
                        m_statName = params->stat;
                    }
                    if (delayParams->find("calc") != delayParams->end()) {
                        params->calc = std::any_cast<std::string>(delayParams->at("calc"));
                        m_calcName = params->calc;
                    }
                    if (delayParams->find("dura") != delayParams->end()) {
                        params->dura = std::any_cast<int64_t>(delayParams->at("dura"));
                    }
                    if (delayParams->find("max") != delayParams->end()) {
                        params->max = std::any_cast<int64_t>(delayParams->at("max"));
                    }
                    if (delayParams->find("min") != delayParams->end()) {
                        params->min = std::any_cast<int64_t>(delayParams->at("min"));
                    }
                    if (delayParams->find("avg") != delayParams->end()) {
                        params->avg = std::any_cast<int64_t>(delayParams->at("avg"));
                    }
                    if (delayParams->find("stut") != delayParams->end()) {
                        params->stut = std::any_cast<double>(delayParams->at("stut"));
                        m_stutterRatio = params->stut;
                    }
                    if (delayParams->find("slop") != delayParams->end()) {
                        params->slop = std::any_cast<double>(delayParams->at("slop"));
                    }
                    if (delayParams->find("speed") != delayParams->end()) {
                        params->speed = std::any_cast<double>(delayParams->at("speed"));
                    }
                    if (delayParams->find("edura") != delayParams->end()) {
                        params->edura = std::any_cast<int64_t>(delayParams->at("edura"));
                    }
                    if (delayParams->find("elimit") != delayParams->end()) {
                        params->elimit = std::any_cast<int64_t>(delayParams->at("elimit"));
                    }
                    if (delayParams->find("rdura") != delayParams->end()) {
                        params->rdura = std::any_cast<int64_t>(delayParams->at("rdura"));
                    }
                    if (delayParams->find("rlimit") != delayParams->end()) {
                        params->rlimit = std::any_cast<int64_t>(delayParams->at("rlimit"));
                    }
                    if (delayParams->find("watch") != delayParams->end()) {
                        params->watch = std::any_cast<int64_t>(delayParams->at("watch"));
                    }
                    addDelayParams(params);
                }
                break;
            }
            case HJ_PLUGIN_NOTIFY_PLUGIN_SETINFOS:
            {
                auto infos = std::make_shared<HJPlayerPluginInfos>();
                infos->audio_dropping = ntf->getInt64("audio_dropping");
                infos->video_dropping = ntf->getInt64("video_dropping");
                infos->audio_decoder = ntf->getInt64("audioDecoder");
                infos->audio_render = ntf->getInt64("audioRender");
                infos->video_main_decoder = ntf->getInt64("videoMainDecoder");
                infos->video_main_render = ntf->getInt64("videoMainRender");
                infos->video_soft_decoder = ntf->getInt64("videoSoftDecoder");
                infos->video_soft_render = ntf->getInt64("videoSoftRender");
                infos->time = HJCurrentSteadyMS();
                HJFLogi("setInfo audio_dropping:{}, video_dropping:{}, audioRender:{}", infos->audio_dropping, infos->video_dropping, infos->audio_render);
                addPluginInfos(infos);
                break;
            }
        }
        return HJ_OK;
    };
    (*param)["playerListener"] = playerListener;

    auto audioInfo = std::make_shared<HJAudioInfo>();
    audioInfo->m_samplesRate = 48000;
    audioInfo->setChannels(2);
    audioInfo->m_sampleFmt = 1;
    audioInfo->m_bytesPerSample = 2;
    (*param)["audioInfo"] = audioInfo;
    (*param)["InsIdx"] = (int)HJMakeGlobalID();

    HJInstanceSettings settings;
    //settings.pluginInfosEnable = true;
    m_player->setSettings(settings);

    m_player->init(param);
 //   auto coreUrl = HJUtilitys::getCoreUrl(m_mediaUrl);
	//auto suffix = HJUtilitys::getSuffix(coreUrl);
    auto localUrl = HJMediaUtils::makeLocalUrl("E:/movies/blob/", m_mediaUrl); //HJFMT("E:/movies/blob/{}{}", HJUtilitys::hash(m_mediaUrl), suffix);
    HJMediaUrl::Ptr mediaUrl = nullptr;
    if (HJFileUtil::fileExist(localUrl.c_str())) {
        mediaUrl = std::make_shared<HJMediaUrl>(localUrl);
    } else {
        mediaUrl = std::make_shared<HJMediaUrl>(m_mediaUrl);
        (*mediaUrl)["local_url"] = localUrl;
    }
    m_player->openURL(mediaUrl);

    return;
}

void HJGraphPlayerView::onClosePlayer()
{
    if(m_player) {
        m_player->done();
        m_player = nullptr;
    }
    //m_player = nullptr;
}

void HJGraphPlayerView::onClickPrepare()
{
    if (m_mediaUrl.empty()) {
        return;
    }
    onClosePlayer();
    onCreatePlayer();

    //HJMainExecutor()->async([&]() {
    //    onClickPrepare();
    //}, 2000);


//    HJFLogi("entry");
//    int res = HJ_OK;
//    HJTicker ticker;
//    do
//    {
//        done();
//        
//        HJFLogi("create player");
//        //
//        HJMediaUrlVector mediaUrls;
//        //HJMediaUrl::Ptr url0 = std::make_shared<HJMediaUrl>("E:/js/1718781990_2.ts");
//        //HJMediaUrl::Ptr url1 = std::make_shared<HJMediaUrl>("E:/movies/720x1280.mp4");
//        //HJMediaUrl::Ptr url2 = std::make_shared<HJMediaUrl>("E:/movies/qiyiboshi2.mp4");
//        //url->setLoopCnt(2);
//        //mediaUrls = { url0, url1, url2 };
////        HJMediaUrl::Ptr url0 = std::make_shared<HJMediaUrl>("E:/js/_LC_AL2_non_27013848617200603950012820_OX.flv");
//        //HJMediaUrl::Ptr url0 = std::make_shared<HJMediaUrl>("E:/js/587102.crdownload.flv");
//        std::string url = m_mediaUrl;
//        if (m_isAsyncUrl) {
//            url = "exasync:" + url;
//        }
//        HJMediaUrl::Ptr url0 = std::make_shared<HJMediaUrl>(url);
//        url0->setTimeout(m_timeout);
////        url0->setLoopCnt(100);
//        mediaUrls = { url0 };
//        //
//        
//
//
//        //m_player->setMediaFrameListener([&](const HJMediaFrame::Ptr mvf) -> int {
//        //    if (mvf->isVideo()) {
//        //        //HJFLogi("recv media frame:" + mvf->formatInfo());
//        //        HJ_AUTO_LOCK(m_mutex);
//        //        m_mvf = mvf;
//        //        if (m_thumbnail) {
//        //            onWriteThumb(m_mvf);
//        //        }
//        //    }
//        //    return HJ_OK;
//        //});
//        //m_player->setSourceFrameListener([&](const HJMediaFrame::Ptr mavf) -> int {
//        //    if (m_showMuxer) {
//        //        onMuxer(mavf);
//        //    }
//        //    if (m_showPusher) {
//        //        onPusher2(mavf);
//        //    }
//        //    return HJ_OK;
//        //});
//        //res = m_player->init(mediaUrls, [&](const HJNotification::Ptr ntf) -> int {
//        //    //HJLogi("notify begin, id:" + HJNotifyIDToString((HJNotifyID)ntf->getID()) + ", value:" + HJ2STR(ntf->getVal()) + ", msg:" + ntf->getMsg());
//        //    switch (ntf->getID())
//        //    {
//        //    case HJNotify_Prepared:
//        //    {
//        //        auto minfo = ntf->getValue<HJMediaInfo::Ptr>(HJMediaInfo::KEY_WORLDS);
//        //        if (minfo) {
//        //            setMediaInfo(minfo);
//        //        }
//        //        HJMainExecutorAsync([=]() {
//        //            onStart();
//        //            });
//        //        break;
//        //    }
//        //    case HJNotify_NeedWindow:
//        //    {
//        //        onSetWindow();
//        //        break;
//        //    }
//        //    case HJNotify_ProgressStatus:
//        //    {
//        //        auto progInfo = ntf->getValue<HJProgressInfo::Ptr>(HJProgressInfo::KEY_WORLDS);
//        //        if (progInfo) {
//        //            HJ_AUTO_LOCK(m_mutex);
//        //            m_progInfo = progInfo;
//        //        }
//        //        break;
//        //    }
//        //    case HJNotify_Already:
//        //    {
//        //        onClickPlay("play");
//        //        break;
//        //    }
//        //    case HJNotify_DemuxEnd:
//        //    {
//        //        break;
//        //    }
//        //    case HJNotify_Complete:
//        //    {
//        //        onCompleted();
//        //        break;
//        //    }
//        //    case HJNotify_Error:
//        //    {
//        //        int ret = onProcessError(ntf->getVal());
//        //        if (HJ_OK != ret) {
//        //            HJFLoge("error, on process error failed, m_tryCnt:{}", m_tryCnt);
//        //            break;
//        //        }
//        //        HJMainExecutorAsync([=]() {
//        //            onClickPrepare();
//        //        });
//        //        break;
//        //    }
//        //    case HJNotify_MediaChanged:
//        //    {
//        //        auto minfo = ntf->getValue<HJStreamInfo::Ptr>(HJStreamInfo::KEY_WORLDS);
//        //        if (minfo) {
//        //            HJ_AUTO_LOCK(m_mutex);
//        //            m_minfos.emplace_back(minfo);
//        //        }
//        //        break;
//        //    }
//        //    default: {
//        //        break;
//        //    }
//        //    }
//        //    return HJ_OK;
//        //});
//    } while (false);
//    HJFLogi("end, time:{}", ticker.elapsedTime());

    return;
}

void HJGraphPlayerView::onClickPlay(const std::string& tips)
{
    if (!m_player /*|| !m_player->isReady()*/) {
        return;
    }
    //if ("play" == tips) {
    //    m_player->play();
    //}
    //else {
    //    m_player->pause();
    //}
    //HJMainExecutor()->async([&] {
    //    onClickPrepare();
    //}, HJUtilitys::random(2000, 5000));

    return;
}

void HJGraphPlayerView::onStart()
{
    if (!m_player) {
        return;
    }
    //m_player->start();

    return;
}

void HJGraphPlayerView::onSetWindow()
{
    if (!m_player) {
        return;
    }
    //m_player->setWindow((HJHND)m_player.get());

    return;
}

void HJGraphPlayerView::onSeek(const int64_t pos)
{
    if (!m_player) {
        return;
    }
    HJFLogi("entry, pos:{}", pos);
    //m_player->seek(pos);

    return;
}

void HJGraphPlayerView::onCompleted()
{
    HJFLogi("entry");
    HJMainExecutor()->async([=]() {
        onClickPrepare();
    }, 1000);
}

void HJGraphPlayerView::onWriteThumb(const HJMediaFrame::Ptr& frame)
{
    int res = HJ_OK;
    //if (!m_imageWriter) {
    //    m_imageWriter = std::make_shared<HJImageWriter>();
    //    res = m_imageWriter->initWithPNG(180, 320);
    //    if (HJ_OK != res) {
    //        HJFLoge("error, create image writer failed:{}", res);
    //        return;
    //    }
    //    HJFileUtil::delete_file(m_thumbUrl.c_str());
    //    if (!HJFileUtil::is_dir(m_thumbUrl.c_str())) {
    //        HJFileUtil::makeDir(m_thumbUrl.c_str());
    //    }
    //}
    //if (m_mvf && m_imageWriter) {
    //    std::string thumbUrl = HJFMT("{}/{}_{}x{}.png", m_thumbUrl, m_imageIdx, m_mvf->getVideoInfo()->m_width, m_mvf->getVideoInfo()->m_height);
    //    res = m_imageWriter->writeFrame(m_mvf, thumbUrl);
    //    m_imageIdx++;
    //}
}

int HJGraphPlayerView::onProcessError(const int err)
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

int HJGraphPlayerView::onMuxer(const HJMediaFrame::Ptr& frame)
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

int HJGraphPlayerView::onPusher(const HJMediaFrame::Ptr& frame)
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

int HJGraphPlayerView::onPusher2(const HJMediaFrame::Ptr& frame)
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
            (*options)[HJRTMPUtils::STORE_KEY_LOCALURL] = localUrl;
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
        res = m_rtmpMuxer->writeFrame(frame);
        if (HJ_OK != res) {
            break;
        }
    } while (false);
    return res;
}

void HJGraphPlayerView::onClickMute(const std::string& tips)
{
    if (!m_player /*|| !m_player->isReady()*/) {
        return;
    }
    if ("speaker" == tips) {
        //m_player->setVolume(0.0f);
    }
    else {
        //m_player->setVolume(1.0f);
    }
}

void HJGraphPlayerView::onClickVolume(const float volume)
{
    if (!m_player /*|| !m_player->isReady()*/) {
        return;
    }
    HJFLogi("set volume:{}", volume);
    //m_player->setVolume(volume);
}

void HJGraphPlayerView::onTest()
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

void HJGraphPlayerView::onTest1()
{
    HJLogi("onTest1 --  entry");
//    m_sem.wait();
    //m_sem.waitBFor(HJSeconds(10));
//    HJLogi("onTest1 --  end");
    //HJDefaultExecutor()->async([&]() {
    //    HJLogi("onTest1  -- async entry");
    //    });
}

void HJGraphPlayerView::onTestHLS()
{
    HJMediaUrl::Ptr url = std::make_shared<HJMediaUrl>("https://live-replay-5.test.huajiao.com/psegments/z1.huajiao-live.HJ_0_qiniu_5_huajiao-live__h265_45752749_1730776617184_3550_T/1730776662-1730776884.m3u8");
    //m_hlsParser = std::make_shared<HJHLSParser>();
    //int ret = m_hlsParser->init(url);

    return;
}

void HJGraphPlayerView::onTestByteBuffer()
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

void HJGraphPlayerView::onNetBlock(const std::string& tips)
{
    if(tips == "block") {
        HJGlobalSettingsManager::setNetBlockEnable(false);
    } else {
        HJGlobalSettingsManager::setNetBlockEnable(true);
    }

    if (m_rtmpMuxer) 
    {
        if (tips == "block") {
            m_rtmpMuxer->setNetBlock(false);
        }
        else {
            m_rtmpMuxer->setNetBlock(true);
        }
    }

    return;
}
void HJGraphPlayerView::onDataBlock(const std::string& tips)
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

void HJGraphPlayerView::onNetBitrate(int bitrate)
{
    if (!m_rtmpMuxer) {
        return;
    }
    m_rtmpMuxer->setNetSimulater(bitrate);
    return;
}

HJGraphBPlotInfos HJGraphPlayerView::getDelayParams()
{
    HJ_AUTOU_LOCK(m_mutex);
    HJGraphBPlotInfos infos;
    if (m_delayParams.size() <= 0) {
        return infos;
    }
    double xsMin = INT64_MAX;
    double xsMax = 0;
    double ysMax = 0;
    int64_t sampleCount = m_delayParams.size();
    int64_t sampleMax = HJ_MIN(3 * 1000, m_delayParams.size());

    HJBuffer::Ptr xsbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    HJBuffer::Ptr ysbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    HJBuffer::Ptr avgbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    //HJBuffer::Ptr maxbuffer = std::make_shared<HJBuffer>(m_delayParams.size() * sizeof(float));
    //HJBuffer::Ptr minbuffer = std::make_shared<HJBuffer>(m_delayParams.size() * sizeof(float));
    HJBuffer::Ptr upperbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    HJBuffer::Ptr lowerbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    float* xs = (float*)xsbuffer->data();
    float* ys = (float*)ysbuffer->data();
    float* avg = (float*)avgbuffer->data();
    //float* max = (float*)maxbuffer->data();
    //float* min = (float*)minbuffer->data();
    float* upper = (float*)upperbuffer->data();
    float* lower = (float*)lowerbuffer->data();

    HJBuffer::Ptr speedbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    float* speed = (float*)speedbuffer->data();
    HJBuffer::Ptr stuterbuffer = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    float* stutter = (float*)stuterbuffer->data();

    HJBuffer::Ptr audiodropping = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    float* ad_data = (float*)audiodropping->data();
    HJBuffer::Ptr videodropping = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    float* vd_data = (float*)videodropping->data();
    HJBuffer::Ptr audiorender = std::make_shared<HJBuffer>(sampleMax * sizeof(float));
    float* ar_data = (float*)audiorender->data();
    //infos->audio_dropping = ntf->getInt64("audio_dropping");
    //infos->video_dropping = ntf->getInt64("video_dropping");
    //infos->audio_decoder = ntf->getInt64("audioDecoder");
    //infos->audio_render = ntf->getInt64("audioRender");
    //infos->video_main_decoder = ntf->getInt64("videoMainDecoder");
    //infos->video_main_render = ntf->getInt64("videoMainRender");
    //infos->video_soft_decoder = ntf->getInt64("videoSoftDecoder");
    //infos->video_soft_render = ntf->getInt64("videoSoftRender");

    size_t j = HJ_MAX(0, sampleCount - sampleMax);
    for (size_t i = 0; i < sampleMax; i++, j++)
    {
        const auto& val = m_delayParams[j];
        xs[i] = val->watch;
        if (xs[i] > xsMax) {
            xsMax = xs[i];
        }
        if (xs[i] < xsMin) {
            xsMin = xs[i];
        }
        ys[i] = val->dura;
        if (ys[i] > ysMax) {
            ysMax = ys[i];
        }
        avg[i] = val->edura;
        if (avg[i] > ysMax) {
            ysMax = avg[i];
        }
        //max[i] = val->max;
        //min[i] = val->min;
        upper[i] = val->rdura;
        if (upper[i] > ysMax) {
            ysMax = upper[i];
        }
        lower[i] = val->rlimit;
        //
        speed[i] = val->speed;
        stutter[i] = val->stut * 10.0;

        auto plugins = m_pluginInfos.size() > j ? m_pluginInfos[j] : std::make_shared<HJPlayerPluginInfos>();
		ad_data[i] = plugins->audio_dropping;
        vd_data[i] = plugins->video_dropping;
        ar_data[i] = plugins->audio_render;
    }
    infos.xsbuffer = xsbuffer;
    infos.xsMin = xsMin;
    infos.xsMax = std::fmax(xsMax, 1000.0);
    infos.ysbuffers["dura"] = ysbuffer;
    infos.ysMaxs["dura"] = ysMax;
    infos.size = sampleMax;
    infos.ysbuffers["avg"] = avgbuffer;
    //infos.ysbuffers["max"] = maxbuffer;
    //infos.ysbuffers["min"] = minbuffer;
    infos.ysbuffers["upper"] = upperbuffer;
    infos.ysbuffers["lower"] = lowerbuffer;

    infos.ysbuffers["speed"] = speedbuffer;
    infos.ysbuffers["stutter"] = stuterbuffer;

    infos.ysbuffers["ad_data"] = audiodropping;
    infos.ysbuffers["vd_data"] = videodropping;
    infos.ysbuffers["ar_data"] = audiorender;

    return infos;
}


NS_HJ_END
