#import "AppDelegate.h"

#import "HJPlayerContext.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    (void)application;
    (void)launchOptions;

    HJPlayerContextConfig *config = [[HJPlayerContextConfig alloc] init];
    config.logEnabled = YES;
    config.logDirectory = NSTemporaryDirectory();
    config.logMode = HJPlayerLogModeConsole | HJPlayerLogModeFile;
    config.logMaxFileSize = 5 * 1024 * 1024;
    config.logMaxFileCount = 5;

    NSInteger result = [[HJPlayerContext sharedInstance] startWithConfig:config];
    if (result < 0) {
        NSLog(@"HJPlayerContext start failed: %ld", (long)result);
    }
    return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application {
    (void)application;
    [[HJPlayerContext sharedInstance] stop];
}

@end
