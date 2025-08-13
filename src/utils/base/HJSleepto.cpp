#include <stdbool.h>
#include "HJSleepto.h"
#include "HJTime.h"

NS_HJ_BEGIN

HJSleepto::HJSleepto()
{

}
HJSleepto::~HJSleepto()
{

}

#if defined(WIN32_LIB)

#include <windows.h>
static bool have_clockfreq = false;
static LARGE_INTEGER clock_freq;

static inline uint64_t get_clockfreq(void)
{
	if (!have_clockfreq) {
		QueryPerformanceFrequency(&clock_freq);
		have_clockfreq = true;
	}

	return clock_freq.QuadPart;
}
uint64_t HJSleepto::Gettime100ns()
{
	LARGE_INTEGER current_time;
	double time_val;

	QueryPerformanceCounter(&current_time);
	time_val = (double)current_time.QuadPart;
	time_val *= 10000000.0;
	time_val /= (double)get_clockfreq();

	return (uint64_t)time_val;
}

bool HJSleepto::Sleepto100ns(uint64_t time_target)
{
	uint64_t t = Gettime100ns();
	uint32_t milliseconds;

	if (t >= time_target)
		return false;

	milliseconds = (uint32_t)((time_target - t) / 10000);
	if (milliseconds > 1)
		Sleep(milliseconds - 1);

	for (;;) {
		t = Gettime100ns();
		if (t >= time_target)
			return true;

		Sleep(0);
	}
}

#else

uint64_t HJSleepto::Gettime100ns(void)
{
	return 0;
}

bool HJSleepto::Sleepto100ns(uint64_t time_target)
{
	return false;
}

#endif

int64_t HJSleepto::sleepTo(int64_t i_targetTime)
{
    uint64_t t = HJCurrentSteadyMS();
    if (t >= i_targetTime)
		return 0;
    return i_targetTime - t;
}

NS_HJ_END