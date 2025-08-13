#pragma once


#include "HJComObject.h"
#include "HJComVideoCapture.h"
#include "HJSPBuffer.h"
#include <GLES3/gl3.h>

#ifndef WIN32
#define LONG long
#endif

#define LLONG	long long
typedef LONG (*HJPlayerMsgCallBack)(LONG  lHandle, LONG  lCommand, LLONG para1, LLONG para2, LONG  dwUser);

using JSinglePlayerInitFunc = void*(*)(const std::string &i_info, HJPlayerMsgCallBack i_cb, void *user, bool i_bOpenLog);
using JSinglePlayerSetWindowFunc = int(*)(void *handle, void *window, int width, int height, int i_nState);
using JSinglePlayerDoneFunc = void(*)(void *i_handle);

NS_HJ_BEGIN

typedef enum SLComVideoAlphaVideoNotify
{
    SLComVideoAlphaVideoNotify_Error,
    SLComVideoAlphaVideoNotify_Complete,
    //SLComVideoAlphaGiftNotify_AsynceClosePlayerEnd,
} SLComVideoAlphaVideoNotify;

class SLComVideoAlphaVideo : public HJComVideoCapture
{
public:
    HJ_DEFINE_CREATE(SLComVideoAlphaVideo);
    SLComVideoAlphaVideo();
    virtual ~SLComVideoAlphaVideo();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    void sHJPlayerMsgCallBack(int64_t command, int64_t para1, int64_t para2);
    //void closePlayer();
protected:
    virtual int  doRenderUpdate() override;
    virtual int  doRenderDraw(int i_targetWidth, int i_targetHeight) override;
private:
    static void *m_libFFHandle;
    static void *m_libHJPlayerHandle;
    
    void *m_handle = nullptr;
    
    JSinglePlayerInitFunc m_funcInit = nullptr;
    JSinglePlayerSetWindowFunc m_funcSetWindow = nullptr;
    JSinglePlayerDoneFunc m_funcDone = nullptr;
    
    //SLThreadPool::Ptr m_closePlayerThread = nullptr;
};

NS_HJ_END



