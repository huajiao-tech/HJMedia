//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaFrame.h"

typedef struct OH_AVCodec OH_AVCodec;
typedef struct OH_AVFormat OH_AVFormat;
typedef struct OH_AVBuffer OH_AVBuffer;
typedef struct OH_AVCodecBufferAttr OH_AVCodecBufferAttr;

NS_HJ_BEGIN
//***********************************************************************************//
typedef int (*HJOHCodecFreeAVBuffer)(OH_AVCodec *codec, uint32_t index);    //OH_AVErrCode
/**
 * int64_t pts;  //us
 * int32_t size;
 * int32_t offset;
 * uint32_t flags;  //AVCODEC_BUFFER_FLAGS_EOS
 */
class HJOHAVBufferInfo : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJOHAVBufferInfo);
    HJOHAVBufferInfo() = default;
    HJOHAVBufferInfo(OH_AVBuffer* buffer, uint32_t index);
    HJOHAVBufferInfo(OH_AVCodec* codec, OH_AVBuffer* buffer, uint32_t index, HJOHCodecFreeAVBuffer freeFunc);
    virtual ~HJOHAVBufferInfo();

    int setBufferAttr(uint8_t* data, int size, int64_t pts, uint32_t flags = 0);
private:
    OH_AVCodec* m_codec = NULL;
    OH_AVBuffer* m_buffer = NULL;
    std::shared_ptr<OH_AVCodecBufferAttr> m_attr = nullptr; // {0, 0, 0, AVCODEC_BUFFER_FLAGS_NONE};
    HJOHCodecFreeAVBuffer m_freeFunc = NULL;
};

//***********************************************************************************//
class HJOHACodecUserData : public HJObject 
{
public:
    HJ_DECLARE_PUWTR(HJOHACodecUserData);
    HJOHACodecUserData(const std::weak_ptr<HJObject>& parent, const std::string& name) 
        : HJObject(name)
        , m_parent(parent) {};

    HJObject::Ptr getParent() const {
        if (m_parent.expired()) {
            return nullptr;
        }
        return m_parent.lock();
    }
    size_t inputQueueSize() {
        HJ_AUTOU_LOCK(inputMutex);
        return inputQueue.size();
    }
    HJOHAVBufferInfo::Ptr getInputBuffer() {
        HJ_AUTOU_LOCK(inputMutex);
        if (inputQueue.size() == 0) {
            return nullptr;
        }
        auto buf = inputQueue.front();
        inputQueue.pop_front();
//        HJLogi(getName() + ", get input buffer size:" + HJ2STR(inputQueue.size()) + ", index:" + HJ2STR(buf->getID()));
        return buf;
    }
    void setInputBuffer(const HJOHAVBufferInfo::Ptr& buf) {
        HJ_AUTOU_LOCK(inputMutex);
        inputQueue.push_back(buf);
//        HJLogi(getName() + ", set input buffer size:" + HJ2STR(inputQueue.size()) + ", index:" + HJ2STR(buf->getID()));
    }
    size_t outputQueueSize() {
        HJ_AUTOU_LOCK(outputMutex);
        return outputQueue.size();
    }
    HJOHAVBufferInfo::Ptr getOutputBuffer() {
        HJ_AUTOU_LOCK(outputMutex);
        if (outputQueue.size() == 0) {
            return nullptr;
        }
        auto buf = outputQueue.front();
        outputQueue.pop_front();
        HJLogi(getName() + ", get output buffer size:" + HJ2STR(inputQueue.size()) + ", index:" + HJ2STR(buf->getID()));
        return buf;
    }
    void setOutputBuffer(const HJOHAVBufferInfo::Ptr& buf) {
        HJ_AUTOU_LOCK(outputMutex);
        outputQueue.push_back(buf);
        HJLogi(getName() + ", set output buffer size:" + HJ2STR(inputQueue.size()) + ", index:" + HJ2STR(buf->getID()));
    }
    void setOutputFrames(const HJMediaFrame::Ptr& frame) {
        HJ_AUTOU_LOCK(m_frameMutex);
        m_outputFrames.push_back(frame);
//        HJLogi(getName() + ", set output frame size:" + HJ2STR(m_outputFrames.size()) + ", frame:" + frame->formatInfo());
    }
    HJMediaFrame::Ptr getOutputFrame() {
        HJ_AUTOU_LOCK(m_frameMutex);
        if (m_outputFrames.size() == 0) {
            return nullptr;
        }
        auto frame = m_outputFrames.front();
        m_outputFrames.pop_front();
//        HJLogi(getName() + ", get output frame size:" + HJ2STR(m_outputFrames.size()) + ", frame:" + frame->formatInfo());
        return frame;
    }
    void clear() {
        inputQueue.clear();
        outputQueue.clear();
        m_outputFrames.clear();
    }
protected:
    std::weak_ptr<HJObject> m_parent;
    //
    std::mutex inputMutex;
    std::deque<HJOHAVBufferInfo::Ptr> inputQueue;
    //
    std::mutex outputMutex;
    std::deque<HJOHAVBufferInfo::Ptr> outputQueue;
    //
    std::mutex m_frameMutex;
    std::deque<HJMediaFrame::Ptr> m_outputFrames;
};

//***********************************************************************************//
class HJOHCodecUtils
{
public:
    static const char* mapCodecIdToMime(int codecId);
    static const int mapAVSampleFormatToOH(int format);
};

NS_HJ_END