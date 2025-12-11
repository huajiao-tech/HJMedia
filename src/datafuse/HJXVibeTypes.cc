//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXVibeTypes.h"
#include "HJFLog.h"
#include "HJPackerManager.h"

NS_HJ_BEGIN
//***********************************************************************************//
bool HJVibeData::verifyXVibHeader(const uint8_t* data, size_t size)
{
    if (!data || size < HJXVIB_HEADER_SIZE) return false;
    HJXVibHeader h;
    if (!HJPackerManager::readHeader(data, size, h)) {
        HJFLoge("invalid header");
        return false;
    }
    //    
    if (HJXVIB_VERSION != h.version) return false;
    if (h.compress >= HJCompressType::COMPRESS_MAX) return false;
    if (h.data_type >= HJXVibDataType::VIBDATA_MAX) return false;
    if (0 != h.reserved) return false;
    if (HJXVIB_HEADER_SIZE != h.data_offset) return false;
    if (h.seg_id >= h.seg_count) return false;
    return true;
}

NS_HJ_END