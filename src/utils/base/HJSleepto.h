#pragma once

#include "HJPrerequisites.h"
#include <stdint.h>

NS_HJ_BEGIN

class HJSleepto 
{
public:
    HJSleepto();
    virtual ~HJSleepto();

    static uint64_t Gettime100ns();
    static bool Sleepto100ns(uint64_t time_target);
    static int64_t sleepTo(int64_t i_targetTime);
};

NS_HJ_END