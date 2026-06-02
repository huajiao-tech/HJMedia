#pragma once

#include "HJPrerequisites.h"
#include <functional>
#include <mutex>
#include <memory>


NS_HJ_BEGIN

typedef enum HJMediaRenderConfigType
{
    HJMediaRenderConfigType_None = 0,
    HJMediaRenderConfigType_SourceRender,

} HJMediaRenderConfigType;

class HJMediaRenderConfig
{
public:
	static std::string GetConfig(HJMediaRenderConfigType type);

    static std::string m_configSourceRender;
};

NS_HJ_END
