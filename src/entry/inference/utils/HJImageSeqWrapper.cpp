#include "HJImageSeqWrapper.h"

#include "HJError.h"
#include "HJFLog.h"
#include "HJTime.h"
#include "HJTransferMediaData.h"
#include "stb_image.h"
#include "HJThreadPool.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

NS_HJ_USING

namespace
{
bool sIsImageFile(const std::filesystem::path& p)
{
    if (!p.has_extension())
    {
        return false;
    }
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return (ext == ".png") || (ext == ".jpg") || (ext == ".jpeg") || (ext == ".bmp") || (ext == ".webp");
}

int sNaturalCompare(const std::string& a, const std::string& b)
{
    size_t i = 0;
    size_t j = 0;
    while (i < a.size() && j < b.size())
    {
        const bool aIsDigit = std::isdigit(static_cast<unsigned char>(a[i])) != 0;
        const bool bIsDigit = std::isdigit(static_cast<unsigned char>(b[j])) != 0;
        if (aIsDigit && bIsDigit)
        {
            size_t aStart = i;
            size_t bStart = j;
            while (aStart < a.size() && a[aStart] == '0')
            {
                aStart++;
            }
            while (bStart < b.size() && b[bStart] == '0')
            {
                bStart++;
            }

            size_t aEnd = aStart;
            size_t bEnd = bStart;
            while (aEnd < a.size() && std::isdigit(static_cast<unsigned char>(a[aEnd])) != 0)
            {
                aEnd++;
            }
            while (bEnd < b.size() && std::isdigit(static_cast<unsigned char>(b[bEnd])) != 0)
            {
                bEnd++;
            }

            const size_t aLen = aEnd - aStart;
            const size_t bLen = bEnd - bStart;
            if (aLen != bLen)
            {
                return aLen < bLen ? -1 : 1;
            }
            if (aLen > 0)
            {
                const int cmp = a.compare(aStart, aLen, b, bStart, bLen);
                if (cmp != 0)
                {
                    return cmp < 0 ? -1 : 1;
                }
            }

            size_t aRunEnd = aEnd;
            size_t bRunEnd = bEnd;
            while (aRunEnd < a.size() && std::isdigit(static_cast<unsigned char>(a[aRunEnd])) != 0)
            {
                aRunEnd++;
            }
            while (bRunEnd < b.size() && std::isdigit(static_cast<unsigned char>(b[bRunEnd])) != 0)
            {
                bRunEnd++;
            }

            const size_t aRunLen = aRunEnd - i;
            const size_t bRunLen = bRunEnd - j;
            if (aRunLen != bRunLen)
            {
                return aRunLen < bRunLen ? -1 : 1;
            }

            i = aRunEnd;
            j = bRunEnd;
            continue;
        }

        const unsigned char aCh = static_cast<unsigned char>(a[i]);
        const unsigned char bCh = static_cast<unsigned char>(b[j]);
        const char aLower = static_cast<char>(std::tolower(aCh));
        const char bLower = static_cast<char>(std::tolower(bCh));
        if (aLower != bLower)
        {
            return aLower < bLower ? -1 : 1;
        }
        if (a[i] != b[j])
        {
            return a[i] < b[j] ? -1 : 1;
        }
        i++;
        j++;
    }

    if (i == a.size() && j == b.size())
    {
        return 0;
    }
    return i == a.size() ? -1 : 1;
}
}

HJImageSeqWrapper::HJImageSeqWrapper()
{
}

HJImageSeqWrapper::~HJImageSeqWrapper()
{
    priDone();
}

int HJImageSeqWrapper::priParseConfigLocked(const std::string& seqPath)
{
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    const std::filesystem::path dir(seqPath);

    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
    {
        HJFLoge("HJImageSeqWrapper invalid seq dir:{}", seqPath);
        return HJErrInvalidPath;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (ec)
        {
            HJFLoge("HJImageSeqWrapper iterate dir failed path:{} err:{}", seqPath, ec.message());
            return HJErrInvalidPath;
        }
        if (!entry.is_regular_file())
        {
            continue;
        }
        const std::filesystem::path p = entry.path();
        if (!sIsImageFile(p))
        {
            continue;
        }
        files.push_back(p);
    }

    if (files.empty())
    {
        HJFLoge("HJImageSeqWrapper no image files in:{}", seqPath);
        return HJErrInvalidPath;
    }

    std::sort(files.begin(), files.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
        return sNaturalCompare(a.filename().string(), b.filename().string()) < 0;
    });

    m_pngUrlQueue.clear();
    m_pngUrlQueue.reserve(files.size());
    for (const auto& p : files)
    {
        m_pngUrlQueue.push_back(p.string());
    }

    m_pngIdx = 0;
    return HJ_OK;
}

int HJImageSeqWrapper::priLoadCurrentFrameLocked()
{
    if (m_pngUrlQueue.empty())
    {
        return HJ_OK;
    }

    if (m_pngIdx >= static_cast<int>(m_pngUrlQueue.size()))
    {
        if (!m_bLoop)
        {
            HJFLogi("HJImageSeqWrapper decode finished single pass");
            return HJ_OK;
        }
        m_pngIdx = 0;
    }

    const std::string& url = m_pngUrlQueue[m_pngIdx];
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* decoded = stbi_load(url.c_str(), &width, &height, &channels, 4);
    (void)channels;
    if (!decoded || width <= 0 || height <= 0)
    {
        HJFLoge("HJImageSeqWrapper stbi_load failed url:{}", url);
        if (decoded)
        {
            stbi_image_free(decoded);
        }
        m_pngIdx++;
        return HJErrInvalidFile;
    }

    unsigned char* rgbaData[4] = { decoded, nullptr, nullptr, nullptr };
    int rgbaPitch[4] = { width * 4, 0, 0, 0 };
    HJTransferMediaData::Ptr frame = HJTransferMediaData::create(
        HJConvertDataType_RGBA,
        rgbaData,
        rgbaPitch,
        width,
        height,
        HJCurrentSteadyMS(),
        m_outputType);
    stbi_image_free(decoded);
    if (!frame)
    {
        m_pngIdx++;
        return HJErrInvalidResult;
    }

    m_cache.enqueue(frame);
    m_pngIdx++;
    return HJ_OK;
}

int HJImageSeqWrapper::priOnSchedule()
{
    int ret = priLoadCurrentFrameLocked();
    if (ret < 0)
    {
        HJFLoge("HJImageSeqWrapper load frame failed ret:{}", ret);
    }
    return HJ_OK;
}

int HJImageSeqWrapper::init(const std::string& seqPath, int fps, HJConvertDataType outputType, bool i_bLoop)
{
    if (seqPath.empty() || fps <= 0)
    {
        return HJErrInvalidParams;
    }

    int ret = priParseConfigLocked(seqPath);
    if (ret < 0)
    {
        return ret;
    }
    m_fps = fps;
    m_outputType = outputType;
    m_bLoop = i_bLoop;

    m_timer = HJTimerThreadPool::Create();
    if (!m_timer)
    {
        return HJErrNotInited;
    }

    const int64_t intervalMs = std::max<int64_t>(1, 1000 / m_fps);
    ret = m_timer->startTimer(intervalMs, [this]() { return priOnSchedule(); }, false);
    if (ret < 0)
    {
        m_timer = nullptr;
        m_cache.clear();
        return ret;
    }
    return HJ_OK;
}

std::shared_ptr<HJTransferMediaData> HJImageSeqWrapper::acquire()
{
    return m_cache.acquire();
}

bool HJImageSeqWrapper::recovery(const std::shared_ptr<HJTransferMediaData>& data)
{
    if (!data)
    {
        return false;
    }
    return m_cache.recovery(data);
}

void HJImageSeqWrapper::priDone()
{
    if (m_timer)
    {
        m_timer->stopTimer();
        m_timer = nullptr;
    }
    m_pngUrlQueue.clear();
    m_cache.clear();
}
