#pragma once

#include "HJTypesMacro.h"
#include "HJJson.h"

NS_HJ_BEGIN

class OpenPlayerInfo final : public HJInterpreter {
public:
    std::string url;
    std::string localDir;
    std::string rid;
    int fps = 30;
    int repeats = 0;
    int videoCodecType;
    int sourceType;
    int playerType;
    bool bSplitScreenMirror = false;
    int disableMFlag = 0;
    std::string m_graphConfig;
    std::map<std::string, bool> m_enableSEIUUids;

    HJ_JSON_REFLECT_BEGIN(OpenPlayerInfo)
        HJ_JSON_REFLECT_MEMBER(url)
        HJ_JSON_REFLECT_MEMBER(localDir)
        HJ_JSON_REFLECT_MEMBER(rid)
        HJ_JSON_REFLECT_MEMBER(fps)
        HJ_JSON_REFLECT_MEMBER(repeats)
        HJ_JSON_REFLECT_MEMBER(videoCodecType)
        HJ_JSON_REFLECT_MEMBER(sourceType)
        HJ_JSON_REFLECT_MEMBER(playerType)
        HJ_JSON_REFLECT_MEMBER(bSplitScreenMirror)
        HJ_JSON_REFLECT_MEMBER(disableMFlag)
        HJ_JSON_REFLECT_MEMBER(m_graphConfig)
        HJ_JSON_REFLECT_MEMBER(m_enableSEIUUids)
    HJ_JSON_REFLECT_END(OpenPlayerInfo)
};

HJ_JSON_REFLECT_IMPLEMENT(OpenPlayerInfo)

NS_HJ_END
