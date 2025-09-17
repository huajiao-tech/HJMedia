#pragma once

#include "HJPrerequisites.h"
#include "HJBaseCodec.h"
#include "HJDataDequeue.h"
#include "HJRegularProc.h"

typedef struct OH_AVCodec OH_AVCodec;
typedef struct OH_AVFormat OH_AVFormat;
typedef struct OH_AVBuffer OH_AVBuffer;

NS_HJ_BEGIN

class HJBSFParser;
class HJH2645Parser;
class HJVDecOHCodec;

class HJVDecOHCodecResObj
{
public:
    HJ_DEFINE_CREATE(HJVDecOHCodecResObj);
    HJVDecOHCodecResObj()
    {
    }
    HJVDecOHCodecResObj(std::shared_ptr<HJVDecOHCodec> i_ptr, int i_key):
        m_wtr(i_ptr)
        ,m_key(i_key)
    {
    }
    virtual ~HJVDecOHCodecResObj();
    void setRender(bool i_bRender = true)
    {
        m_bRender = i_bRender;
    }
    std::weak_ptr<HJVDecOHCodec> m_wtr;
    int m_key = 0;
    bool m_bRender = true;
};

class HJVDecOHCodecInfo
{
public:
    HJ_DEFINE_CREATE(HJVDecOHCodecInfo)
    HJVDecOHCodecInfo()
    {

    }
    HJVDecOHCodecInfo(uint32_t i_idx, OH_AVBuffer *i_buffer):
            m_index(i_idx)
          , m_buffer(i_buffer)
    {

    }
    virtual ~HJVDecOHCodecInfo()
    {

    }
    static std::shared_ptr<HJVDecOHCodecInfo> Create(uint32_t i_idx, OH_AVBuffer *i_buffer) 
    {      
        return std::make_shared<HJVDecOHCodecInfo>(i_idx, i_buffer); 
    } 
    uint32_t m_index = 0;
    OH_AVBuffer *m_buffer = nullptr;
};

//typedef struct HOVideoDecStat
//{
//    int64_t m_openTime = 0;
//    int64_t m_createTime = 0;
//    int64_t m_configureTime = 0;
//    int64_t m_prepareTime = 0;
//    int64_t m_startTime = 0;
//    int64_t m_firstInput = 0;
//    int64_t m_inputCnt = 0;
//    int64_t m_firstOutPut = 0;
//} HOVideoDecStat;

typedef enum HJVDecOHCodecState
{
    HJVDecOHCodecState_idle,
    HJVDecOHCodecState_ready,
    HJVDecOHCodecState_inEOF,
    HJVDecOHCodecState_outEOF,
    HJVDecOHCodecState_error,
}HJVDecOHCodecState;

class HJVDecOHCodec : public HJBaseCodec
{
public:
//    using HJVDecOHCodecNotify = std::function<void()>;
    using HOOutDataMap = std::unordered_map<int64_t, uint32_t>;
    using HOOutDataMapIt = HOOutDataMap::iterator;
    
    HJ_DEFINE_CREATE(HJVDecOHCodec)
    
    HJVDecOHCodec();
    virtual ~HJVDecOHCodec();
    
    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
    virtual void done() override;
    int renderOutput(bool i_bRender, int64_t i_outputKey);
    bool isCanInput();
private:
      
    static void OnCodecError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    
    int priRenderOutput(bool i_bRender, int64_t i_key);
    void priRenderOutCache(int64_t i_key, uint32_t i_idx);
    void priClearMapOnly(); 
    
    void priOnCodecError(OH_AVCodec *codec, int32_t errorCode);
    void priOnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format);
    void priOnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer);
    void priOnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer);

    void priDone();
    bool m_bDone = false;
    
    OH_AVCodec *m_decoder = nullptr;
    HJSpDataDeque<HJVDecOHCodecInfo::Ptr> m_inputQueue;
    HJSpDataDeque<HJVDecOHCodecInfo::Ptr> m_outputQueue;
    std::shared_ptr<HJBSFParser> m_parser = nullptr;
    
    HOOutDataMap m_map;
    int64_t m_outputKey = 0;
    std::mutex m_mutex;

    HJVDecOHCodecState m_state = HJVDecOHCodecState_idle;
    
    std::shared_ptr<HJH2645Parser> m_headerParser = nullptr;
    int m_mapSize = 0;
    HJRegularProcTimer m_regularLog;  
//    int64_t m_lastdts = 0;
//    SL::SLRegularProcTimer m_regularTimer;
//    HOVideoDecStat m_stat;
    
    HJRunnable m_inputBufferListener{};
    HJRunnable m_outputBufferListener{};
};

NS_HJ_END



