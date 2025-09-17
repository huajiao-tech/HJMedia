#pragma once

#include "HJPrerequisites.h"
#include "HJCommonInterface.h"

NS_HJ_BEGIN

class HJEntryContext
{
public:
    HJ_DEFINE_CREATE(HJEntryContext);
    HJEntryContext();
    virtual ~HJEntryContext();
    static int init(HJEntryType i_type, const HJEntryContextInfo& i_contextInfo);
    static void unInit();
    
private:
    static bool m_bContextInit;
    
    static void onHandleFFLogCallback(void* avcl, int level, const char *fmt, va_list vl);
};

NS_HJ_END