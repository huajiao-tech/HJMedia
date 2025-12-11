//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJDataFuse.h"
#include "HJFLog.h"
#include "HJPackerManager.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJDataFuse::HJDataFuse(const HJParams::Ptr& params) 
	: m_params(params) 
{
	m_packerManager = HJCreateu<HJPackerManager>(params);
	m_packerManager->registerPackers<HJZlibPacker, HJSegPacker>();
}

HJDataFuse::~HJDataFuse() 
{
	m_packerManager = nullptr;
}

std::vector<HJRawBuffer> HJDataFuse::pack(const HJRawBuffer& data)
{
	if (!m_packerManager) {
		return {};
	}
	HJVibeData::Ptr vibeData = HJCreates<HJVibeData>(data);
	auto outData = m_packerManager->pack(vibeData);
	if(!outData) {
		return {};
	}

	std::vector<HJRawBuffer> ret{};
	for (size_t i = 0; i < outData->size(); i++)
	{
		auto userData = outData->get(i);
		ret.push_back(userData->getData());
	}
	return ret;
}

std::vector<HJRawBuffer> HJDataFuse::pack(const std::string& data)
{
	HJRawBuffer inData = HJUtilitys::makeBytes(data);
	return pack(inData);
}

HJRawBuffer HJDataFuse::unpack(const std::vector<HJRawBuffer>& data)
{
	if (!m_packerManager) {
		return {};
	}
	HJVibeData::Ptr vibeData = HJCreates<HJVibeData>();
	for (auto& it : data) {
		HJUserData::Ptr userData = HJCreateu<HJUserData>(it);
		vibeData->append(userData);
	}
	HJRawBuffer ret{};
	auto outData = m_packerManager->unpack(vibeData);
	for (size_t i = 0; i < outData->size(); i++)
	{
		auto& userData = outData->get(i);
		ret.insert(ret.end(), userData->getData().begin(), userData->getData().end());
	}
	return ret;
}

NS_HJ_END