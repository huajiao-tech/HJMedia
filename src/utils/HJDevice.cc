//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJDevice.h"
#if defined(HJ_OS_WINDOWS)
#include <windows.h>
#elif defined(HJ_OS_DARWIN)
#import <mach/mach.h>
#import <mach/mach_host.h>
#include "osys/HJISysUtils.h"
#endif

NS_HJ_BEGIN
//***********************************************************************************//
#if defined(HJ_OS_WINDOWS)
typedef union HJWOSTimeData {
	FILETIME ft;
	unsigned long long val;
}HJWOSTimeData;

class HJCpuUsageInfo : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJCpuUsageInfo>;

	HJWOSTimeData m_lastTime = { 0 };
	HJWOSTimeData m_lastSysTime = { 0 };
	HJWOSTimeData m_LastUserTime = { 0 };
	DWORD m_coreCount = 0;
};
#endif

//***********************************************************************************//
HJCpuUsage::HJCpuUsage()
{
	init();
}

int HJCpuUsage::init()
{
	int res = HJ_OK;
#if defined(HJ_OS_WINDOWS)
	m_cpuInfo = std::make_shared<HJCpuUsageInfo>();
	SYSTEM_INFO si;
	FILETIME dummy;

	GetSystemInfo(&si);
	GetSystemTimeAsFileTime(&m_cpuInfo->m_lastTime.ft);
	GetProcessTimes(GetCurrentProcess(), &dummy, &dummy,
		&m_cpuInfo->m_lastSysTime.ft, &m_cpuInfo->m_LastUserTime.ft);
	m_cpuInfo->m_coreCount = si.dwNumberOfProcessors;
#endif
	return res;
}

double HJCpuUsage::queryUsage()
{
	double usage = 0.0;
#if defined(HJ_OS_WINDOWS)
	HJWOSTimeData cur_time, cur_sys_time, cur_user_time;
	FILETIME dummy;
	double percent = 0.0;

	GetSystemTimeAsFileTime(&cur_time.ft);
	GetProcessTimes(GetCurrentProcess(), &dummy, &dummy, &cur_sys_time.ft,
		&cur_user_time.ft);

	percent = (double)(cur_sys_time.val - m_cpuInfo->m_lastSysTime.val +
		(cur_user_time.val - m_cpuInfo->m_LastUserTime.val));
	percent /= (double)(cur_time.val - m_cpuInfo->m_lastTime.val);
	percent /= (double)m_cpuInfo->m_coreCount;

	m_cpuInfo->m_lastTime.val = cur_time.val;
	m_cpuInfo->m_lastSysTime.val = cur_sys_time.val;
	m_cpuInfo->m_LastUserTime.val = cur_user_time.val;

	usage = percent * 100.0;
#elif defined(HJ_OS_DARWIN)
	kern_return_t           kr;
	thread_array_t          threadList;         // ���浱ǰMach task���߳��б�
	mach_msg_type_number_t  threadCount;        // ���浱ǰMach task���̸߳���
	thread_info_data_t      threadInfo;         // ���浥���̵߳���Ϣ�б�
	mach_msg_type_number_t  threadInfoCount;    // ���浱ǰ�̵߳���Ϣ�б���С
	thread_basic_info_t     threadBasicInfo;    // �̵߳Ļ�����Ϣ

	// ͨ����task_threads��API���û�ȡָ�� task ���߳��б�
	//  mach_task_self_����ʾ��ȡ��ǰ�� Mach task
	kr = task_threads(mach_task_self(), &threadList, &threadCount);
	if (kr != KERN_SUCCESS) {
		return -1;
	}
	double cpuUsage = 0;
	for (int i = 0; i < threadCount; i++) {
		threadInfoCount = THREAD_INFO_MAX;
		// ͨ����thread_info��API��������ѯָ���̵߳���Ϣ
		//  flavor����������THREAD_BASIC_INFO��ʹ��������ͻ᷵���̵߳Ļ�����Ϣ��
		//  ������ thread_basic_info_t �ṹ�壬�������û���ϵͳ������ʱ�䡢����״̬�͵������ȼ���
		kr = thread_info(threadList[i], THREAD_BASIC_INFO, (thread_info_t)threadInfo, &threadInfoCount);
		if (kr != KERN_SUCCESS) {
			return -1;
		}

		threadBasicInfo = (thread_basic_info_t)threadInfo;
		if (!(threadBasicInfo->flags & TH_FLAGS_IDLE)) {
			cpuUsage += threadBasicInfo->cpu_usage;
		}
	}

	// �����ڴ棬��ֹ�ڴ�й©
	vm_deallocate(mach_task_self(), (vm_offset_t)threadList, threadCount * sizeof(thread_t));

	usage = cpuUsage / (double)TH_USAGE_SCALE * 100.0;
#endif
	return usage;
}

//***********************************************************************************//
float HJDevice::getCpuUsage()
{
    float usage = 0.0f;
#if defined(HJ_OS_DARWIN)
    usage = HJISysUtils::getCpuUsageX();
#else
#endif
    return usage;
}

float HJDevice::getGpuUsage()
{
    float usage = 0.0f;
#if defined(HJ_OS_DARWIN)
    usage = HJISysUtils::getGpuUsage();
#else
#endif
    return usage;
}

std::tuple<float, float> HJDevice::getMemoryUsage()
{
    std::tuple<float, float> usage(0.0f, 0.0f);
#if defined(HJ_OS_DARWIN)
    usage = HJISysUtils::getMemoryUsage();
#else
#endif
    return usage;
}

NS_HJ_END
