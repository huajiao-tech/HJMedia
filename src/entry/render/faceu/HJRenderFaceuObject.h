#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"

NS_HJ_BEGIN

class HJRteComSourceFaceu;

class HJRenderFaceuObject : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRenderFaceuObject);
    HJRenderFaceuObject();
    virtual ~HJRenderFaceuObject();
    int init(HJBaseParam::Ptr i_param);
    int render(int i_width, int i_height);
private:
    std::shared_ptr<HJRteComSourceFaceu> m_faceu = nullptr;
};

NS_HJ_END





