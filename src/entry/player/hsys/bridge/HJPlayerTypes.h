#pragma once

#include "HJTypesMacro.h"
#include "HJJson.h"

NS_HJ_BEGIN

class OpenPlayerInfo final : public HJInterpreter {
public:
    OpenPlayerInfo(HJYJsonObject::Ptr obj) : HJInterpreter(obj) {}

    HJ_JSON_AUTO_SERIAL_DESERIAL(url, fps, videoCodecType, sourceType, playerType, bSplitScreenMirror, disableMFlag)

    std::string url;
    int fps = 30;
    int videoCodecType;
    int sourceType;
    int playerType;
    bool bSplitScreenMirror = false;
    int disableMFlag = 0;
};

NS_HJ_END