#import <Foundation/Foundation.h>

@protocol HJLifeCycleDelegate <NSObject>

@optional

- (void)didBecomeActive:(NSNotification*)notification;
- (void)willResignActive:(NSNotification*)notification;
- (void)didEnterBackground:(NSNotification*)notification;
- (void)willEnterForeground:(NSNotification*)notification;
- (void)willTerminate:(NSNotification*)notification;
- (void)didReceiveMemoryWarning:(NSNotification*)notification;
- (BOOL)applicationOpenURL:(NSURL *)url
                   options:(NSDictionary<NSString *, id> *)options;
- (void)audioSessionInterrupt:(NSNotification *)notification;
- (void)audioSessionRouteChange:(NSNotification *)notification;
- (void)dataWillBecomeUnavailable:(NSNotification *)notification;

@end


@interface HJLifeCycle : NSObject<HJLifeCycleDelegate>

+ (void)registerLifeCycleListener:(id<HJLifeCycleDelegate>)obj;
+ (void)unregisterLifeCycleListener:(id<HJLifeCycleDelegate>)obj;
+ (BOOL)dispatchOpenURL:(NSURL *)url
                options:(NSDictionary<NSString *, id> *)options;

@end
