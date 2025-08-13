#include "HJPluginCodec.h"
#include "HJFLog.h"

NS_HJ_BEGIN

int HJPluginCodec::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJStreamInfo::Ptr, streamInfo);
	if (!streamInfo) {
		return HJErrInvalidParams;
	}
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(bool, createThread);
	GET_PARAMETER(HJListener, pluginListener);
	auto param = std::make_shared<HJKeyStorage>();
	(*param)["thread"] = thread;
	(*param)["createThread"] = createThread;
	if (pluginListener) {
		(*param)["pluginListener"] = pluginListener;
	}
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	do {
		ret = initCodec(streamInfo);
		if (ret < 0) {
			break;
		}

		return HJ_OK;
	} while (false);

	HJPluginCodec::internalRelease();
	return ret;
}

void HJPluginCodec::internalRelease()
{
	if (m_codec != nullptr) {
		m_codec->done();
		m_codec = nullptr;
	}

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

		return HJ_OK;
	} while (false);

	m_codec = nullptr;
	m_status = HJSTATUS_Exception;
	return ret;
}

NS_HJ_END
