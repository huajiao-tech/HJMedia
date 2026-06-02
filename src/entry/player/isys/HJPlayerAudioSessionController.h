#import <Foundation/Foundation.h>

#import "HJLifeCycle.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, HJPlayerAudioSessionPauseReason) {
    HJPlayerAudioSessionPauseReasonUnknown = 0,
    HJPlayerAudioSessionPauseReasonInterruption = 1,
    HJPlayerAudioSessionPauseReasonRouteChange = 2,
};

typedef NS_ENUM(NSInteger, HJPlayerAudioSessionResumeReason) {
    HJPlayerAudioSessionResumeReasonUnknown = 0,
    HJPlayerAudioSessionResumeReasonInterruptionEnded = 1,
};

@protocol HJPlayerAudioSessionControllerOwner <NSObject>

- (BOOL)hj_playerAudioSessionControllerIsPlaying;
- (NSInteger)hj_playerAudioSessionControllerPauseForReason:
    (HJPlayerAudioSessionPauseReason)reason;
- (NSInteger)hj_playerAudioSessionControllerResumeForReason:
    (HJPlayerAudioSessionResumeReason)reason;

@end

@interface HJPlayerAudioSessionController : NSObject <HJLifeCycleDelegate>

- (instancetype)initWithOwner:(id<HJPlayerAudioSessionControllerOwner>)owner
           manageAudioSession:(BOOL)manageAudioSession
  pausePlayerForAudioSessionEvents:(BOOL)pausePlayerForAudioSessionEvents
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

- (void)startObserving;
- (void)stopObserving;
- (BOOL)prepareForPlayback:(NSError * _Nullable *)error;
- (void)handlePlayerDidPause;
- (void)handlePlayerDidResume;
- (void)handlePlayerDidStop;
- (void)handlePlayerDidDestroy;

@property (nonatomic, assign, readonly, getter=isManagingAudioSession) BOOL managingAudioSession;
@property (nonatomic, assign, readonly) BOOL pausePlayerForAudioSessionEvents;
@property (nonatomic, assign, readonly, getter=isObserving) BOOL observing;

@end

NS_ASSUME_NONNULL_END
