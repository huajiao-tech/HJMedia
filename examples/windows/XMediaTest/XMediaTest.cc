//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN         //��ֹ windows.h ���� winsock.h, ������WinSock2.h��ͻ
#endif
#include "HJUtils.h"
#include "HJMedias.h"
#include "HJCores.h"
#include "HJGuis.h"
#include "HJImguiUtils.h"
#include "HJFLog.h"
#include "HJNetManager.h"
#include "HJLocalServer.h"
#include "HJGlobalSettings.h"
#include "HJMov2Live.h"
#include "HJContext.h"

#include "gtest/gtest.h"

#if defined(HJ_OS_WIN32)
#include <crtdbg.h>
#elif defined(HJ_OS_MACOS)
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#endif

using namespace HJ;

// Forward declaration of the function that will run the tests.
void runHJExecutorPoolTests(int argc, const char* argv[]) {
    HJFLogi("Running HJExecutorPool Tests...");
    // Initialize Google Test. It requires a non-const char**, so we use const_cast.
    // This is a common practice when interfacing with libraries that don't use const correctly.
    ::testing::InitGoogleTest(&argc, const_cast<char**>(argv));

    // Set a filter to run only the tests from the HJExecutorPoolTest suite.
    // The wildcard '*' ensures all tests within this suite are run.
    ::testing::GTEST_FLAG(filter) = "HJExecutorPoolTest*";

    // Run the tests
    int test_result = RUN_ALL_TESTS();
    HJFLogi("Test run finished with result: {}", test_result);
}

int main(int argc, const char* argv[])
{
#if defined(HJ_OS_WIN32)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#elif defined(HJ_OS_MACOS)
//    @autoreleasepool
//    {
//        NSApp = [NSApplication sharedApplication];
//        AppDelegate* delegate = [[AppDelegate alloc] init];
//        [[NSApplication sharedApplication] setDelegate:delegate];
//    }
#endif
    HJContextConfig cfg;
    cfg.mIsLog = true;
    cfg.mLogMode = HJLOGMODE_FILE | HJLOGMODE_CONSOLE;
    cfg.mLogDir = HJUtilitys::logDir() + R"(xmediatools/)";
    cfg.mResDir = HJConcateDirectory(HJCurrentDirectory(), R"(resources)");
#if defined(HJ_OS_WIN32)
    cfg.mMediaDir = "E:/movies";
    //cfg.mResDir = HJUtils::exeDir() + "resources";
    //cfg.mMediaDir = "C:";
#elif defined(HJ_OS_MACOS)
    //cfg.mResDir = "/Users/zhiyongleng/works/MediaTools/SLMedia_OW/examples/Windows/XMediaTools/resources";
    cfg.mMediaDir = "/Users/zhiyongleng/works/movies";
#endif
    cfg.mThreadNum = 0;
    //HJFileUtil::delete_file((cfg.mLogDir + "HJLog.txt").c_str());
    HJContext::Instance().init(cfg);

    runHJExecutorPoolTests(argc, argv);

     //HJContext::Instance().done();
#if defined(HJ_OS_MACOS)
    return NSApplicationMain(argc, argv);
#else
    return HJ_OK;
#endif
}
