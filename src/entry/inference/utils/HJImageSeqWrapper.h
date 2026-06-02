#pragma once

#include "HJExport.h"
#include "HJAsyncCache.h"
#include "HJConvertUtils.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace HJ
{
    class HJTransferMediaData;
    class HJTimerThreadPool;
}

class HJExport HJImageSeqWrapper
{
public:
    HJImageSeqWrapper();
    virtual ~HJImageSeqWrapper();

    int init(
        const std::string& seqPath,
        int fps = 15,
        HJConvertDataType outputType = HJConvertDataType_RGBA,
        bool i_bLoop = true);

    std::shared_ptr<HJ::HJTransferMediaData> acquire();
    bool recovery(const std::shared_ptr<HJ::HJTransferMediaData>& data);
   

private:
    void priDone();
    int priOnSchedule();
    int priLoadCurrentFrameLocked();
    int priParseConfigLocked(const std::string& seqPath);

    std::shared_ptr<HJ::HJTimerThreadPool> m_timer = nullptr;
    HJ::HJAsyncCache<std::shared_ptr<HJ::HJTransferMediaData>> m_cache;
    std::vector<std::string> m_pngUrlQueue;
    int m_pngIdx = 0;
    int m_fps = 30;
    HJConvertDataType m_outputType = HJConvertDataType_YUVNV12;
    bool m_bLoop = true;
};
