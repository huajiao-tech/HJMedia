#pragma once

#include <string>
#include <vector>
#include "HJPrerequisites.h"

NS_HJ_BEGIN

class HJCoreVersion
{
public:
	HJCoreVersion() = delete;
	virtual ~HJCoreVersion() = delete;
	static const std::string& getVersion();
	static const std::string& getVersionInfo();
	static const std::string getVersionDetail();
private:
	static std::string m_emptyVersion;
	static std::vector<std::tuple<std::string, std::string>> m_versionVector;
};

NS_HJ_END
