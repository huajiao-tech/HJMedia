#pragma once

#include "HJComObject.h"
#include "HJMediaFrame.h"

NS_HJ_BEGIN

class HJComMediaFrameCvt 
{
public:
    static HJComMediaFrame::Ptr getComFrame(int i_flag, HJMediaFrame::Ptr i_frame);
    static HJMediaFrame::Ptr getFrame(HJComMediaFrame::Ptr i_frame);
};

NS_HJ_END



