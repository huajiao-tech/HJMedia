#include "HJComMediaFrameCvt.h"

NS_HJ_BEGIN

HJComMediaFrame::Ptr HJComMediaFrameCvt::getComFrame(int i_flag, HJMediaFrame::Ptr i_frame)
{
	HJComMediaFrame::Ptr outFrame = HJComMediaFrame::Create();
	outFrame->SetFlag(i_flag);
	(*outFrame)[HJ_CatchName(HJMediaFrame::Ptr)] = i_frame;
	return outFrame;
}

HJMediaFrame::Ptr HJComMediaFrameCvt::getFrame(HJComMediaFrame::Ptr i_frame)
{
	HJMediaFrame::Ptr frame = nullptr;
 	if (i_frame->contains(HJ_CatchName(HJMediaFrame::Ptr)))
	{
		frame = i_frame->getValue<HJMediaFrame::Ptr>(HJ_CatchName(HJMediaFrame::Ptr));
	}
	return frame;
}

NS_HJ_END



