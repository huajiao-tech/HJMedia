#import "HJLifeCycle.h"
#import <UIKit/UIApplication.h>
#import <AVFoundation/AVFoundation.h>

@implementation HJLifeCycle

+ (NSHashTable<id<HJLifeCycleDelegate>> *)openURLListeners {
    static NSHashTable<id<HJLifeCycleDelegate>> *listeners = nil;
    @synchronized(self) {
        if (listeners == nil) {
            listeners = [NSHashTable weakObjectsHashTable];
        }
    }
    return listeners;
}

+ (void)registerObserver:(id<HJLifeCycleDelegate>)obj
                selector:(SEL)selector
        notificationName:(NSNotificationName)notification_name {
    if ([obj respondsToSelector:selector]) {
        [[NSNotificationCenter defaultCenter] addObserver:obj
                                                 selector:selector
                                                     name:notification_name
                                                   object:nil];
    }
}

+ (void)registerLifeCycleListener:(id<HJLifeCycleDelegate>)obj {
    if (obj == nil) {
        return;
    }

    [self unregisterLifeCycleListener:obj];

    [self registerObserver:obj
                  selector:@selector(didBecomeActive:)
          notificationName:UIApplicationDidBecomeActiveNotification];
    [self registerObserver:obj
                  selector:@selector(willResignActive:)
          notificationName:UIApplicationWillResignActiveNotification];
    [self registerObserver:obj
                  selector:@selector(didEnterBackground:)
          notificationName:UIApplicationDidEnterBackgroundNotification];
    [self registerObserver:obj
                  selector:@selector(willEnterForeground:)
          notificationName:UIApplicationWillEnterForegroundNotification];
    [self registerObserver:obj
                  selector:@selector(willTerminate:)
          notificationName:UIApplicationWillTerminateNotification];
    [self registerObserver:obj
                  selector:@selector(didReceiveMemoryWarning:)
          notificationName:UIApplicationDidReceiveMemoryWarningNotification];
    [self registerObserver:obj
                  selector:@selector(audioSessionInterrupt:)
          notificationName:AVAudioSessionInterruptionNotification];
    [self registerObserver:obj
                  selector:@selector(audioSessionRouteChange:)
          notificationName:AVAudioSessionRouteChangeNotification];
    [self registerObserver:obj
                  selector:@selector(dataWillBecomeUnavailable:)
          notificationName:UIApplicationProtectedDataWillBecomeUnavailable];

    if ([obj respondsToSelector:@selector(applicationOpenURL:options:)]) {
        @synchronized(self) {
            [[self openURLListeners] addObject:obj];
        }
    }
}

+ (void)unregisterLifeCycleListener:(id<HJLifeCycleDelegate>)obj {
    if (obj == nil) {
        return;
    }

    NSNotificationCenter *notification_center = [NSNotificationCenter defaultCenter];
    [notification_center removeObserver:obj name:UIApplicationDidBecomeActiveNotification object:nil];
    [notification_center removeObserver:obj name:UIApplicationWillResignActiveNotification object:nil];
    [notification_center removeObserver:obj name:UIApplicationDidEnterBackgroundNotification object:nil];
    [notification_center removeObserver:obj name:UIApplicationWillEnterForegroundNotification object:nil];
    [notification_center removeObserver:obj name:UIApplicationWillTerminateNotification object:nil];
    [notification_center removeObserver:obj name:UIApplicationDidReceiveMemoryWarningNotification object:nil];
    [notification_center removeObserver:obj name:AVAudioSessionInterruptionNotification object:nil];
    [notification_center removeObserver:obj name:AVAudioSessionRouteChangeNotification object:nil];
    [notification_center removeObserver:obj name:UIApplicationProtectedDataWillBecomeUnavailable object:nil];

    @synchronized(self) {
        [[self openURLListeners] removeObject:obj];
    }
}

+ (BOOL)dispatchOpenURL:(NSURL *)url
                options:(NSDictionary<NSString *, id> *)options {
    if (url == nil) {
        return NO;
    }

    NSArray<id<HJLifeCycleDelegate>> *listeners = nil;
    @synchronized(self) {
        listeners = [[[self openURLListeners] allObjects] copy];
    }

    BOOL handled = NO;
    for (id<HJLifeCycleDelegate> listener in listeners) {
        handled = [listener applicationOpenURL:url options:options] || handled;
    }
    return handled;
}

@end
