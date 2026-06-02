#import <AVFAudio/AVFAudio.h>
#import <Foundation/Foundation.h>
#import <objc/runtime.h>

#import "HJAudioSession.h"
#import "HJPlayerAudioSessionController.h"

static NSInteger g_shared_instance_call_count = 0;
static id g_test_audio_session = nil;

@interface HJPlayerAudioSessionControllerTestOwner : NSObject <HJPlayerAudioSessionControllerOwner>

@property (nonatomic, assign) BOOL playing;
@property (nonatomic, assign) NSInteger pauseCount;
@property (nonatomic, assign) NSInteger resumeCount;
@property (nonatomic, assign) HJPlayerAudioSessionPauseReason lastPauseReason;
@property (nonatomic, assign) HJPlayerAudioSessionResumeReason lastResumeReason;

@end

@implementation HJPlayerAudioSessionControllerTestOwner

- (BOOL)hj_playerAudioSessionControllerIsPlaying {
    return self.playing;
}

- (NSInteger)hj_playerAudioSessionControllerPauseForReason:(HJPlayerAudioSessionPauseReason)reason {
    self.pauseCount += 1;
    self.lastPauseReason = reason;
    self.playing = NO;
    return 0;
}

- (NSInteger)hj_playerAudioSessionControllerResumeForReason:(HJPlayerAudioSessionResumeReason)reason {
    self.resumeCount += 1;
    self.lastResumeReason = reason;
    self.playing = YES;
    return 0;
}

@end

@interface HJAudioSession (HJPlayerAudioSessionControllerTestHook)

+ (instancetype)hj_test_sharedInstance;

@end

@interface HJPlayerAudioSessionControllerTestAudioSession : NSObject

@property (nonatomic, assign) NSInteger applyConfigurationCount;
@property (nonatomic, assign) NSInteger setActiveCount;
@property (nonatomic, assign) BOOL lastActiveValue;

@end

static id HJTestSharedInstance(id self, SEL _cmd);
static void HJSwapSharedInstanceImplementations(void);

@implementation HJAudioSession (HJPlayerAudioSessionControllerTestHook)

+ (instancetype)hj_test_sharedInstance {
    return HJTestSharedInstance(self, _cmd);
}

@end

@implementation HJPlayerAudioSessionControllerTestAudioSession

- (BOOL)applyConfiguration:(id)configuration error:(NSError **)error {
    (void)configuration;
    if (error != NULL) {
        *error = nil;
    }
    self.applyConfigurationCount += 1;
    return YES;
}

- (BOOL)setActive:(BOOL)active error:(NSError **)error {
    if (error != NULL) {
        *error = nil;
    }
    self.setActiveCount += 1;
    self.lastActiveValue = active;
    return YES;
}

@end

static id HJTestSharedInstance(id self, SEL _cmd) {
    (void)self;
    (void)_cmd;
    g_shared_instance_call_count += 1;

    if (g_test_audio_session != nil) {
        return g_test_audio_session;
    }

    Class audio_session_class = objc_getClass("HJAudioSession");
    Method test_method = class_getClassMethod(audio_session_class,
                                              @selector(hj_test_sharedInstance));
    IMP original_imp = method_getImplementation(test_method);
    id (*original_fn)(id, SEL) = (id (*)(id, SEL))original_imp;
    return original_fn(audio_session_class, @selector(sharedInstance));
}

static void HJSwapSharedInstanceImplementations(void) {
    Class audio_session_class = objc_getClass("HJAudioSession");
    Method original_method = class_getClassMethod(audio_session_class, @selector(sharedInstance));
    Method test_method = class_getClassMethod(audio_session_class,
                                              @selector(hj_test_sharedInstance));
    method_exchangeImplementations(original_method, test_method);
}

static NSNotification *HJMakeInterruptionNotification(AVAudioSessionInterruptionType type,
                                                      AVAudioSessionInterruptionOptions options) {
    return [NSNotification notificationWithName:AVAudioSessionInterruptionNotification
                                         object:nil
                                       userInfo:@{
                                           AVAudioSessionInterruptionTypeKey: @(type),
                                           AVAudioSessionInterruptionOptionKey: @(options),
                                       }];
}

static NSNotification *HJMakeRouteChangeNotification(AVAudioSessionRouteChangeReason reason) {
    return [NSNotification notificationWithName:AVAudioSessionRouteChangeNotification
                                         object:nil
                                       userInfo:@{
                                           AVAudioSessionRouteChangeReasonKey: @(reason),
                                       }];
}

static int TestManageAudioSessionDisabledSkipsSessionSetup(void) {
    HJSwapSharedInstanceImplementations();
    g_shared_instance_call_count = 0;

    HJPlayerAudioSessionControllerTestOwner *owner =
        [[HJPlayerAudioSessionControllerTestOwner alloc] init];
    HJPlayerAudioSessionController *controller =
        [[HJPlayerAudioSessionController alloc] initWithOwner:owner
                                           manageAudioSession:NO
                              pausePlayerForAudioSessionEvents:NO];
    NSError *error = nil;
    BOOL success = [controller prepareForPlayback:&error];

    HJSwapSharedInstanceImplementations();

    if (!success || error != nil) {
        return 1;
    }
    if (g_shared_instance_call_count != 0) {
        return 2;
    }
    return 0;
}

static int TestControllerDrivenPauseDeactivatesManagedAudioSession(void) {
    HJSwapSharedInstanceImplementations();
    g_shared_instance_call_count = 0;
    HJPlayerAudioSessionControllerTestAudioSession *audio_session =
        [[HJPlayerAudioSessionControllerTestAudioSession alloc] init];
    g_test_audio_session = audio_session;

    HJPlayerAudioSessionControllerTestOwner *owner =
        [[HJPlayerAudioSessionControllerTestOwner alloc] init];
    owner.playing = YES;
    HJPlayerAudioSessionController *controller =
        [[HJPlayerAudioSessionController alloc] initWithOwner:owner
                                           manageAudioSession:YES
                              pausePlayerForAudioSessionEvents:YES];

    NSError *error = nil;
    if (![controller prepareForPlayback:&error] || error != nil) {
        g_test_audio_session = nil;
        HJSwapSharedInstanceImplementations();
        return 3;
    }

    [controller audioSessionInterrupt:HJMakeInterruptionNotification(
                                          AVAudioSessionInterruptionTypeBegan,
                                          0)];
    [controller handlePlayerDidPause];

    g_test_audio_session = nil;
    HJSwapSharedInstanceImplementations();

    if (audio_session.applyConfigurationCount != 1) {
        return 4;
    }
    if (audio_session.setActiveCount != 2) {
        return 5;
    }
    if (audio_session.lastActiveValue) {
        return 6;
    }
    return 0;
}

static int TestInterruptionBeginDoesNotPauseWhenAutoPauseDisabled(void) {
    HJPlayerAudioSessionControllerTestOwner *owner =
        [[HJPlayerAudioSessionControllerTestOwner alloc] init];
    owner.playing = YES;
    HJPlayerAudioSessionController *controller =
        [[HJPlayerAudioSessionController alloc] initWithOwner:owner
                                           manageAudioSession:NO
                              pausePlayerForAudioSessionEvents:NO];

    [controller audioSessionInterrupt:HJMakeInterruptionNotification(
                                          AVAudioSessionInterruptionTypeBegan,
                                          0)];

    if (owner.pauseCount != 0) {
        return 3;
    }
    return 0;
}

static int TestInterruptionBeginPausesWhenAutoPauseEnabled(void) {
    HJPlayerAudioSessionControllerTestOwner *owner =
        [[HJPlayerAudioSessionControllerTestOwner alloc] init];
    owner.playing = YES;
    HJPlayerAudioSessionController *controller =
        [[HJPlayerAudioSessionController alloc] initWithOwner:owner
                                           manageAudioSession:NO
                              pausePlayerForAudioSessionEvents:YES];

    [controller audioSessionInterrupt:HJMakeInterruptionNotification(
                                          AVAudioSessionInterruptionTypeBegan,
                                          0)];

    if (owner.pauseCount != 1) {
        return 4;
    }
    if (owner.lastPauseReason != HJPlayerAudioSessionPauseReasonInterruption) {
        return 5;
    }
    return 0;
}

static int TestInterruptionEndResumesOnlyWhenShouldResumeIsPresent(void) {
    HJPlayerAudioSessionControllerTestOwner *owner =
        [[HJPlayerAudioSessionControllerTestOwner alloc] init];
    owner.playing = YES;
    HJPlayerAudioSessionController *controller =
        [[HJPlayerAudioSessionController alloc] initWithOwner:owner
                                           manageAudioSession:NO
                              pausePlayerForAudioSessionEvents:YES];

    [controller audioSessionInterrupt:HJMakeInterruptionNotification(
                                          AVAudioSessionInterruptionTypeBegan,
                                          0)];
    [controller handlePlayerDidPause];
    [controller audioSessionInterrupt:HJMakeInterruptionNotification(
                                          AVAudioSessionInterruptionTypeEnded,
                                          AVAudioSessionInterruptionOptionShouldResume)];

    if (owner.resumeCount != 1) {
        return 5;
    }
    if (owner.lastResumeReason != HJPlayerAudioSessionResumeReasonInterruptionEnded) {
        return 6;
    }

    HJPlayerAudioSessionControllerTestOwner *owner_without_resume =
        [[HJPlayerAudioSessionControllerTestOwner alloc] init];
    owner_without_resume.playing = YES;
    HJPlayerAudioSessionController *controller_without_resume =
        [[HJPlayerAudioSessionController alloc] initWithOwner:owner_without_resume
                                           manageAudioSession:NO
                              pausePlayerForAudioSessionEvents:YES];
    [controller_without_resume audioSessionInterrupt:HJMakeInterruptionNotification(
                                                        AVAudioSessionInterruptionTypeBegan,
                                                        0)];
    [controller_without_resume handlePlayerDidPause];
    [controller_without_resume audioSessionInterrupt:HJMakeInterruptionNotification(
                                                        AVAudioSessionInterruptionTypeEnded,
                                                        0)];

    if (owner_without_resume.resumeCount != 0) {
        return 7;
    }
    return 0;
}

static int TestRouteChangeOldDeviceUnavailableDoesNotPauseWhenAutoPauseDisabled(void) {
    HJPlayerAudioSessionControllerTestOwner *owner =
        [[HJPlayerAudioSessionControllerTestOwner alloc] init];
    owner.playing = YES;
    HJPlayerAudioSessionController *controller =
        [[HJPlayerAudioSessionController alloc] initWithOwner:owner
                                           manageAudioSession:NO
                              pausePlayerForAudioSessionEvents:NO];

    [controller audioSessionRouteChange:HJMakeRouteChangeNotification(
                                           AVAudioSessionRouteChangeReasonOldDeviceUnavailable)];

    if (owner.pauseCount != 0) {
        return 8;
    }
    return 0;
}

static int TestRouteChangeOldDeviceUnavailablePausesWhenAutoPauseEnabled(void) {
    HJPlayerAudioSessionControllerTestOwner *owner =
        [[HJPlayerAudioSessionControllerTestOwner alloc] init];
    owner.playing = YES;
    HJPlayerAudioSessionController *controller =
        [[HJPlayerAudioSessionController alloc] initWithOwner:owner
                                           manageAudioSession:NO
                              pausePlayerForAudioSessionEvents:YES];

    [controller audioSessionRouteChange:HJMakeRouteChangeNotification(
                                           AVAudioSessionRouteChangeReasonOldDeviceUnavailable)];

    if (owner.pauseCount != 1) {
        return 9;
    }
    if (owner.lastPauseReason != HJPlayerAudioSessionPauseReasonRouteChange) {
        return 10;
    }

    [controller handlePlayerDidPause];
    [controller handlePlayerDidResume];
    return 0;
}

int main(void) {
    @autoreleasepool {
        const int ret1 = TestManageAudioSessionDisabledSkipsSessionSetup();
        if (ret1 != 0) {
            return ret1;
        }

        const int ret2 = TestControllerDrivenPauseDeactivatesManagedAudioSession();
        if (ret2 != 0) {
            return ret2;
        }

        const int ret3 = TestInterruptionBeginDoesNotPauseWhenAutoPauseDisabled();
        if (ret3 != 0) {
            return ret3;
        }

        const int ret4 = TestInterruptionBeginPausesWhenAutoPauseEnabled();
        if (ret4 != 0) {
            return ret4;
        }

        const int ret5 = TestInterruptionEndResumesOnlyWhenShouldResumeIsPresent();
        if (ret5 != 0) {
            return ret5;
        }

        const int ret6 = TestRouteChangeOldDeviceUnavailableDoesNotPauseWhenAutoPauseDisabled();
        if (ret6 != 0) {
            return ret6;
        }

        const int ret7 = TestRouteChangeOldDeviceUnavailablePausesWhenAutoPauseEnabled();
        if (ret7 != 0) {
            return ret7;
        }

        return 0;
    }
}
