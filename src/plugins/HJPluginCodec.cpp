#include "HJPluginCodec.h"
#include "HJFLog.h"

NS_HJ_BEGIN

int HJPluginCodec::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJStreamInfo::Ptr, streamInfo);

	int ret = HJPlugin::internalInit(i_param);
	if (ret < 0) {
		return ret;
	}

	do {
		if (streamInfo) {
			ret = initCodec(streamInfo);
			if (ret < 0) {
				break;
			}
		}

		return HJ_OK;
	} while (false);

	HJPluginCodec::internalRelease();
	return ret;
}

void HJPluginCodec::internalRelease()
{
	releaseCodec();

	HJPlugin::internalRelease();
}

void HJPluginCodec::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}
/*
void HJPluginCodec::flush()
{
	if (m_codec) {
		m_codec->flush();
	}

	HJPlugin::flush();
}
*/
int HJPluginCodec::initCodec(const HJStreamInfo::Ptr& i_streamInfo)
{
	if (!i_streamInfo) {
		return HJErrInvalidParams;
	}

	int ret;
	do {
		releaseCodec();
		m_codec = createCodec();
		if (m_codec == nullptr) {
			ret = HJErrFatal;
			break;
		}

		m_codec->setName(getName());
		ret = m_codec->init(i_streamInfo);
		if (ret < 0) {
			HJFLoge("{}, m_codec->init(), error({})", getName(), ret);
			break;
		}

        //_lfs_
        passThroughReset();
        
		return HJ_OK;
	} while (false);

	releaseCodec();
	return ret;
}

void  HJPluginCodec::releaseCodec()
{
	if (m_codec) {
		m_codec->done();
		m_codec = nullptr;
	}
}

void HJPluginCodec::passThroughSetInput(const HJMediaFrame::Ptr& i_inFrame)
{
    if (i_inFrame)
    {
	    if (i_inFrame->haveStorage("passThroughDemuxSystemTime") && i_inFrame->haveStorage("passThroughIsKey"))
	    {
		    passThroughAdd(i_inFrame->getDTS(), std::move(HJCodecPassThroughInfo::Create<HJCodecPassThroughInfo>(i_inFrame->getValue<int64_t>("passThroughDemuxSystemTime"), i_inFrame->getValue<bool>("passThroughIsKey"))));
	    }        
    }
}

void HJPluginCodec::passThroughSetOutput(const HJMediaFrame::Ptr& i_outFrame)
{
    if (i_outFrame)
    {
  	    int64_t throughKey = i_outFrame->getDTS();
	    HJCodecPassThroughInfo::Ptr throughInfo = passThroughGet(throughKey);
	    if (throughInfo)
	    {
		    (*i_outFrame)["passThroughDemuxSystemTime"] = throughInfo->getDemuxSysTime();
		    (*i_outFrame)["passThroughIsKey"] = throughInfo->isKey();
		    passThroughRemove(throughKey);
            if (throughInfo->isKey())
            {
                HJFLogd("{} passThroughMap after remove isKey:{} map size:{} ", getName(), throughInfo->isKey(), m_passThroughMap.size());
            }
	    }      
    }
}

void HJPluginCodec::passThroughAdd(int64_t i_key, HJCodecPassThroughInfo::Ptr i_info)
{
	m_passThroughMap[i_key] = i_info;
}

HJCodecPassThroughInfo::Ptr HJPluginCodec::passThroughGet(int64_t i_key)
{
	if (m_passThroughMap.find(i_key) != m_passThroughMap.end())
	{
		return m_passThroughMap[i_key];
	}
	return nullptr;
}

void HJPluginCodec::passThroughRemove(int64_t i_key)
{
	if (m_passThroughMap.find(i_key) != m_passThroughMap.end())
	{
		m_passThroughMap.erase(i_key);
	}
}

void HJPluginCodec::passThroughReset()
{
	m_passThroughMap.clear();
}

NS_HJ_END
