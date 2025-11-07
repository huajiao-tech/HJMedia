#pragma once

#include "HJTypesMacro.h"
#include "HJJson.h"

NS_HJ_BEGIN

class PreviewConfig final : public HJInterpreter {
public:
    PreviewConfig(HJYJsonObject::Ptr obj) : HJInterpreter(obj) {}

    HJ_JSON_AUTO_SERIAL_DESERIAL(realWidth, realHeight, previewFps)

    int realWidth;
    int realHeight;
    int previewFps;
};


class VideoConfig final : public HJInterpreter {
public:
    VideoConfig(HJYJsonObject::Ptr obj) : HJInterpreter(obj) {}

    HJ_JSON_AUTO_SERIAL_DESERIAL(codecID, width, height, bitrate, frameRate, gopSize, videoIsROIEnc)

    int codecID;
    int width;
    int height;
    int bitrate;
    int frameRate;
    int gopSize;
    bool videoIsROIEnc;
};


class AudioConfig final : public HJInterpreter {
public:
    AudioConfig(HJYJsonObject::Ptr obj) : HJInterpreter(obj) {}

    HJ_JSON_AUTO_SERIAL_DESERIAL(codecID, bitrate, sampleFmt, samplesRate, channels)

    int codecID;
    int bitrate;
    int sampleFmt;
    int samplesRate;
    int channels;
};

class PusherConfig final : public HJInterpreter {
public:
    PusherConfig(HJYJsonObject::Ptr obj) : HJInterpreter(obj) {}

    int deserialInfo(const HJYJsonObject::Ptr &obj = nullptr) override {
        HJJsonGetObj(videoConfig, VideoConfig);
        HJJsonGetObj(audioConfig, AudioConfig);
        HJJsonGetValue(url);
        return HJ_OK;
    }

    int serialInfo(const HJYJsonObject::Ptr& obj = nullptr) override {
        HJJsonSetObj(videoConfig, VideoConfig);
        HJJsonSetObj(audioConfig, AudioConfig);
        HJJsonSetValue(url);
        return HJ_OK;
    }

    std::shared_ptr<VideoConfig> videoConfig;
    std::shared_ptr<AudioConfig> audioConfig;
    std::string url;
};

NS_HJ_END