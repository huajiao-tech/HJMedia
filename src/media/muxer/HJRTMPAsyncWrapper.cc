//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRTMPAsyncWrapper.h"
#include "HJFLog.h"
#include "HJRTMPUtils.h"

NS_HJ_BEGIN
const int64_t HJRTMPAsyncWrapper::ACQU_TIMEOUT_DEFAULT = 100;  //ms
const int64_t HJRTMPAsyncWrapper::RETRY_INTERVAL_DEFAULT = 100;
const int64_t HJRTMPAsyncWrapper::LOW_BITRATE_LIMITED = 30 * 1000;
//***********************************************************************************//
HJRTMPAsyncWrapper::HJRTMPAsyncWrapper(HJRTMPWrapperDelegate::Wtr delegate)
	: m_delegate(delegate)
{
	setName(HJMakeGlobalName(HJ_TYPE_NAME(HJRTMPAsyncWrapper)));
	m_executor = HJCreateExecutor(HJFMT("{}_{}", getName(), "rtmp_async_wrapper"));
}

HJRTMPAsyncWrapper::~HJRTMPAsyncWrapper()
{
	done();
}

int HJRTMPAsyncWrapper::init(const std::string& url, HJOptions::Ptr opts)
{
	m_url = url;
	m_opts = opts;
	if (m_opts) {
		if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_RETRY_TIME_LIMITED)) {
			m_retryTimeLimited = m_opts->getInt64(HJRTMPUtils::STORE_KEY_RETRY_TIME_LIMITED);
		}
		if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_LOWBR_TIMEOUT_ENABLE)) {
			m_lowBRTimeoutEnable = m_opts->getBool(HJRTMPUtils::STORE_KEY_LOWBR_TIMEOUT_ENABLE);
		}
		if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_LOWBR_TIMEOUT_LIMITED)) {
			m_lowBRTimeoutLimited = m_opts->getInt(HJRTMPUtils::STORE_KEY_LOWBR_TIMEOUT_LIMITED);
		}
		if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_LOWBR_LIMITED)) {
			m_lowBRLimited = m_opts->getInt(HJRTMPUtils::STORE_KEY_LOWBR_LIMITED);
		}
	}
	HJFLogi("entry, url:{}, retryTimeLimited:{}, lowBRTimeoutEnable:{}, lowBRTimeoutLimited:{}, lowBRLimited:{}", url, m_retryTimeLimited, m_lowBRTimeoutEnable, m_lowBRTimeoutLimited, m_lowBRLimited);
	auto running = [&]() {
		return createAVIO();
	};
	m_executor->async(running);

	HJLogi("end");
	return HJ_OK;
}

//ugly, to do async model
void HJRTMPAsyncWrapper::tryWait()
{
	HJLogi("entry");
	auto t0 = HJCurrentSteadyMS();
	HJ_AUTOU_LOCK(m_wrapperMutex);
	int cnt = 0;
	auto now = HJCurrentSteadyMS();
	while (m_isQuit && (HJCurrentSteadyMS() - now) < 2000) { // wait for 2 seconds to send close friendly
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		cnt++;
	}
	if (m_isQuit && m_wrapper) {
		m_wrapper->setQuit();
	}
	HJFLogi("end, time:{}, m_isQuit:{}", HJCurrentSteadyMS() - t0, m_isQuit);
}

void HJRTMPAsyncWrapper::done()
{
	if (!m_executor) {
		return;
	}
	HJLogi("entry");
	auto t0 = HJCurrentSteadyMS();
	m_url = "";
	m_opts = nullptr;

	// tryWait();

	m_executor->sync([&] {
		destroyAVIO();
		notify(std::move(HJMakeNotification(HJRTMP_EVENT_DONE, "rtmp done")));
	});
    HJLogi("sync end");
    m_executor->stop();
	m_executor = nullptr;

	HJFLogi("end, time:{}", HJCurrentSteadyMS() - t0);
	return;
}

int HJRTMPAsyncWrapper::start()
{
	HJLogi("entry");
	m_acquireHeader = true;
	m_isQuit = false;
	m_netBitTracker = HJCreateu<HJNetBitrateTracker>(500);
	//
	auto running = [&]() {
		return run();
	};
	m_executor->async(running, 0, false);
	HJLogi("end");

	return HJ_OK;
}

int HJRTMPAsyncWrapper::run()
{
	if (!m_wrapper) {
		HJLoge("error, no wrapper");
		return HJErrNotAlready;
	}
	auto delegate = HJLockWtr<HJRTMPWrapperDelegate>(m_delegate);
	if (!delegate) {
		HJLoge("error, no delegate");
		return HJErrNotAlready;
	}
	HJLogi("entry");
	int res = HJ_OK;
	int64_t lastTimestamp = 0;
	bool isHevc = false;
	do
	{
		HJBuffer::Ptr tag = delegate->onAcquireMediaTag(HJRTMPAsyncWrapper::ACQU_TIMEOUT_DEFAULT, m_acquireHeader);
		if (tag) {
			m_acquireHeader = false;
			lastTimestamp = tag->getTimestamp();
			if (tag->getName() == "h265") {
				isHevc = true;
			}
			//
			auto t0 = HJCurrentSteadyUS();
			res = m_wrapper->recv();
			if (HJ_OK != res) {
				HJFLoge("error, rtmp socket read:{}", res);
				break;
			}
			auto t1 = HJCurrentSteadyUS();
			res = m_wrapper->send(tag->data(), tag->size(), tag->getID());
			if (HJ_OK != res) {
				HJFLoge("error, rtmp socket write:{}", res);
				break;
			}
#if defined(HJ_HAVE_NETWORK_LIMITED)
			tryNetSimulate(tag->size());
#endif
			auto t2 = HJCurrentSteadyUS();
			if(HJ_NOTS_VALUE == m_runTime) {
				m_runTime = t2;
			}
			auto netkbps = m_netBitTracker->addData(tag->size(), t2 - t1);
            if(netkbps > 0) {
				m_valueStatistics.addValue(netkbps, HJCurrentSteadyMS());
				if (auto minVal = m_valueStatistics.get95thValue(true)) {
//					HJFLogi("sent net kbps:{}, minVal:{}", netkbps, *minVal);
					netkbps = *minVal;
				}
            }
			//
			if ((t2 - t0 > 20000) || (t2 - m_runTime > 1000000)) {
                m_runTime = t2;
				notify(std::move(HJMakeNotification(HJRTMP_EVENT_NET_BITRATE, netkbps, "net kbps")));
				HJFLogw("send end, recv time:{}, send time:{}, netkbps:{}", (t1 - t0)/1000, (t2 - t1)/1000, (int)netkbps);
			}
			//
			if (m_lowBRTimeoutEnable) {
				res = checkNetBitrate(netkbps);
				if (HJ_OK != res) {
					HJFLoge("error, check low net bitrate:{}", netkbps);
					break;
				}
			}
		}
	} while (!m_isQuit);
	//
    HJFLogi("loop end, res:{}, m_isQuit:{}", res, m_isQuit);
	if (HJ_OK == res) {
		m_wrapper->sendFooter(lastTimestamp, isHevc);
	}
	m_wrapper->close();
	m_isQuit = false;
	//
	HJFLogi("end, res:{}, m_isQuit:{}", res, m_isQuit);

	return res;
}

int HJRTMPAsyncWrapper::createAVIO()
{
	if (m_url.empty()) {
		return HJ_EOF;
	}
	m_wrapper = std::make_shared<HJRTMPWrapper>(sharedFrom(this));
	int res = m_wrapper->init(m_url, m_opts);
	if (HJ_OK != res) {
		HJLoge("error, rtmp wrapper init failed");
		return res;
	}

	return HJ_OK;
}

void HJRTMPAsyncWrapper::destroyAVIO()
{
	HJLogi("entry");
	HJ_AUTOU_LOCK(m_wrapperMutex);
	if (m_wrapper) {
		m_wrapper->done();
		m_wrapper = nullptr;
	}
	HJLogi("end");
}

int HJRTMPAsyncWrapper::retryAVIO()
{
	if (HJ_NOTS_VALUE == m_retryTime) {
		m_retryTime = HJCurrentSteadyMS();
	}
	int64_t retryTime = HJCurrentSteadyMS() - m_retryTime;
	//
	HJFLogi("entry, retryTime:{}, m_retryCount:{}, m_retryInterval:{}", retryTime, m_retryCount, m_retryInterval);
	// if (retryTime >= m_retryTimeLimited) {
	// 	HJFLoge("retry time:{}, beyond limited:{}", retryTime, m_retryTimeLimited);
	// 	notify(std::move(HJMakeNotification(HJRTMP_EVENT_RETRY_TIMEOUT, HJFMT("rtmp disconnected, retryTime:{}, m_retryCount:{}", retryTime, m_retryCount))));
	// 	return HJErrRTMPRetry;
	// }
	notify(std::move(HJMakeNotification(HJRTMP_EVENT_RETRY, retryTime, HJFMT("rtmp retryTime:{}, retryCount:{}", retryTime, m_retryCount))));
	m_retryCount++;

	//destroyAVIO();
	int res = createAVIO();
	if (HJ_OK != res) {
		HJFLoge("createAVIO failed, res:{}", res);
		return res;
	}
	HJFLogi("end");
	return HJ_OK;
}

int HJRTMPAsyncWrapper::onRTMPWrapperNotify(HJNotification::Ptr ntf)
{
	if (!ntf) {
		return HJ_OK;
	}
    HJFLogi("begin, notify id:{}, msg:{}", HJRTMPEEventToString((HJRTMPEEvent)ntf->getID()), ntf->getMsg());
	switch (ntf->getID())
	{
		case HJRTMP_EVENT_CONNECT_FAILED:
		case HJRTMP_EVENT_STREAM_CONNECT_FAIL:
		case HJRTMP_EVENT_SEND_Error:
		case HJRTMP_EVENT_RECV_Error:
		{
			auto running = [&]() {
				destroyAVIO();
				//
				return retryAVIO();
			};
			m_retryInterval = getRetryInterval(m_retryCount);
			m_executor->async(running, m_retryInterval, false);
			break;
		}
		case HJRTMP_EVENT_STREAM_CONNECTED:
		{
			m_retryInterval = RETRY_INTERVAL_DEFAULT;
			m_retryTime = HJ_NOTS_VALUE;
			break;
		}
		default: {
			break;
		}
	}
	auto delegate = HJLockWtr<HJRTMPWrapperDelegate>(m_delegate);
    if (delegate) {
        return delegate->onRTMPWrapperNotify(ntf);
    }
	return HJ_OK;
}

HJBuffer::Ptr HJRTMPAsyncWrapper::onAcquireMediaTag(int64_t timeout, bool isHeader)
{
	return nullptr;
}

double HJRTMPAsyncWrapper::getRetryInterval(int n) {
    if (n < 0) {
        return 0.0;
    }
    double interval = pow(1.4933043, n);
    //
    return std::min(interval, 50.0) * HJRTMPAsyncWrapper::RETRY_INTERVAL_DEFAULT;
}

int HJRTMPAsyncWrapper::checkNetBitrate(int netkbps)
{
	if (netkbps <= 0) {
		return HJ_OK;
	}
	//HJFLogi("netkbps:{}", netkbps);
	if (netkbps < m_lowBRLimited)
	{
		if (HJ_NOTS_VALUE == m_lowBitrateTime) {
			m_lowBitrateTime = HJCurrentSteadyMS();
		}
		if (HJ_NOTS_VALUE != m_lowBitrateTime) 
		{
			auto lowBRDuration = HJCurrentSteadyMS() - m_lowBitrateTime;
			if (lowBRDuration > m_lowBRTimeoutLimited)
			{
				m_lowBitrateTime = HJ_NOTS_VALUE;
				notify(std::move(HJMakeNotification(HJRTMP_EVENT_LOW_BITRATE, netkbps, HJFMT("low bitrate:{}", netkbps))));
				HJLogw("warning , low bitrate beyond limited");
				//
				auto running = [&]() {
					destroyAVIO();
					//
					return retryAVIO();
				};
				m_executor->async(running, 0, false);

				return HJErrNetUnkown;
			}
		}
	}
	else {
		m_lowBitrateTime = HJ_NOTS_VALUE;
	}
	return HJ_OK;
}

void HJRTMPAsyncWrapper::tryNetSimulate(int size)
{
#if defined(HJ_HAVE_NETWORK_LIMITED)
	if (m_newSimulator)
	{
		m_newSimulator = false;
		if (m_simulaBitrate > 0) {
			HJFLogi("HJNetworkSimulator m_simulaBitrate:{}", m_simulaBitrate);
			m_simulator = HJCreateu<HJNetworkSimulator>(m_simulaBitrate);
		} else {
			HJFLogi("HJNetworkSimulator m_simulaBitrate:{} close", m_simulaBitrate);
			m_simulator = nullptr;
		}
	}
	if (m_simulator && size > 0) {
		m_simulator->waitData(size);
	}
#endif
}


NS_HJ_END
