#include "HJComUtils.h"

NS_HJ_USING;

namespace {

class HJComUtilsAtomicFormatProbe : public HJBaseObject {
public:
    HJComUtilsAtomicFormatProbe() {
        setInsName("HJComUtilsAtomicFormatProbe");
    }
};

}  // namespace

int HJComUtilsAtomicFormatCompileSmokeTest() {
    HJComUtilsAtomicFormatProbe probe;
    probe.setDebugIdx(7);
    return probe.getDebugIdx();
}
