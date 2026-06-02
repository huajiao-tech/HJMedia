#include "HJCoreVersion.h"

NS_HJ_BEGIN

std::string HJCoreVersion::m_emptyVersion = "unknown";
std::vector<std::tuple<std::string, std::string>> HJCoreVersion::m_versionVector
{
	{"1.0.0", "20251219_faceu_v2"},
	{"1.0.1", "20260126_rendergraph_v3"},
	{"1.0.2", "20260409_musicplayer_audiomixer_v6"},
};

const std::string& HJCoreVersion::getVersion()
{
	if (m_versionVector.empty())
	{
		return m_emptyVersion;
	}

	auto it = m_versionVector.rbegin();
	return std::get<0>(*it);
}
const std::string& HJCoreVersion::getVersionInfo()
{
	if (m_versionVector.empty())
	{
		return m_emptyVersion;
	}

	auto it = m_versionVector.rbegin();
	return std::get<1>(*it);
}
const std::string HJCoreVersion::getVersionDetail()
{
	return getVersion() + "." + getVersionInfo();
}

NS_HJ_END
