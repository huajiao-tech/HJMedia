#import "AppDelegate.h"

#import "ViewController.h"
#include "HJRenderContextExport.h"

@interface AppDelegate ()
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    @autoreleasepool {
        HJEntryContextInfo info;
        info.logIsValid = true;
        info.logDir = std::string([NSTemporaryDirectory() UTF8String]);
        info.logMode = HJLogLMode_CONSOLE;
        info.logMaxFileSize = 5 * 1024 * 1024;
        info.logMaxFileNum = 2;
        int initResult = renderContextInit(info);
        if (initResult != 0) {
            NSLog(@"renderContextInit failed: %d", initResult);
        }
    }
    return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application {
    renderContextUnInit();
}

@end
