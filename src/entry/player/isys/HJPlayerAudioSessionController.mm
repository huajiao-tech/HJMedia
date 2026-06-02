#import "HJPlayerAudioSessionController.h"

#import <AVFAudio/AVFAudio.h>

#import "HJAudioSession.h"

@interface HJPlayerAudioSessionController ()

@property (nonatomic, weak) id<HJPlayerAudioSessionControllerOwner> owner;
@property (nonatomic, assign, readwrite, getter=isManagingAudioSession) BOOL managingAudioSession;
@property (nonatomic, assign, readwrite) BOOL pausePlayerForAudioSessionEvents;
@property (nonatomic, assign, readwrite, getter=isObserving) BOOL observing;

- (void)clearInterruptionState;
- (void)clearPauseState;
- (void)deactivateAudioSessionIfNeeded;

@end

@implementation HJPlayerAudioSessionController {
    BOOL _was_playing_before_interruption;
    BOOL _paused_by_interruption;
    BOOL _paused_by_route_change;
    HJPlayerAudioSessionPauseReason _pending_pause_reason;
}

- (instancetype)initWithOwner:(id<HJPlayerAudioSessionControllerOwner>)owner
           manageAudioSession:(BOOL)manageAudioSession
  pausePlayerForAudioSessionEvents:(BOOL)pausePlayerForAudioSessionEvents {
    self = [super init];
    if (self) {
        _owner = owner;
        _managingAudioSession = manageAudioSession;
        _pausePlayerForAudioSessionEvents = pausePlayerForAudioSessionEvents;
        _observing = NO;
        _was_playing_before_interruption = NO;
        _paused_by_interruption = NO;
        _paused_by_route_change = NO;
        _pending_pause_reason = HJPlayerAudioSessionPauseReasonUnknown;
    }
    return self;
}

- (void)dealloc {
    [self stopObserving];
}

- (void)startObserving {
    if (self.isObserving) {
        return;
    }

    [HJLifeCycle registerLifeCycleListener:self];
    self.observing = YES;
}

- (void)stopObserving {
    if (!self.isObserving) {
        return;
    }

    [HJLifeCycle unregisterLifeCycleListener:self];
    self.observing = NO;
}

- (BOOL)prepareForPlayback:(NSError * _Nullable *)error {
    [self clearPauseState];
    if (!self.isManagingAudioSession) {
        if (error != NULL) {
            *error = nil;
        }
        return YES;
    }

    HJAudioSessionConfiguration *configuration = [[HJAudioSessionConfiguration alloc] init];
    configuration.category = HJAudioSessionCategoryPlayback;
    configuration.mode = HJAudioSessionModeDefault;

    HJAudioSession *session = [HJAudioSession sharedInstance];
    if (![session applyConfiguration:configuration error:error]) {
        return NO;
    }

    return [session setActive:YES error:error];
}

- (void)handlePlayerDidPause {
    if (_pending_pause_reason == HJPlayerAudioSessionPauseReasonInterruption) {
        _paused_by_interruption = YES;
        _pending_pause_reason = HJPlayerAudioSessionPauseReasonUnknown;
    } else if (_pending_pause_reason == HJPlayerAudioSessionPauseReasonRouteChange) {
        _paused_by_route_change = YES;
        _pending_pause_reason = HJPlayerAudioSessionPauseReasonUnknown;
    } else {
        [self clearPauseState];
    }

    [self deactivateAudioSessionIfNeeded];
}

- (void)handlePlayerDidResume {
    [self clearPauseState];
}

- (void)handlePlayerDidStop {
    [self clearPauseState];
    [self deactivateAudioSessionIfNeeded];
}

- (void)handlePlayerDidDestroy {
    [self handlePlayerDidStop];
    [self stopObserving];
}

- (void)audioSessionInterrupt:(NSNotification *)notification {
    id<HJPlayerAudioSessionControllerOwner> owner = self.owner;
    if (owner == nil) {
        [self clearInterruptionState];
        return;
    }

    NSNumber *type_value = notification.userInfo[AVAudioSessionInterruptionTypeKey];
    const AVAudioSessionInterruptionType type =
        (AVAudioSessionInterruptionType)type_value.unsignedIntegerValue;
    if (type == AVAudioSessionInterruptionTypeBegan) {
        [self clearInterruptionState];
        if (!self.pausePlayerForAudioSessionEvents) {
            return;
        }
        if (![owner hj_playerAudioSessionControllerIsPlaying]) {
            return;
        }

        _was_playing_before_interruption = YES;
        _pending_pause_reason = HJPlayerAudioSessionPauseReasonInterruption;
        const NSInteger pause_result =
            [owner hj_playerAudioSessionControllerPauseForReason:
                HJPlayerAudioSessionPauseReasonInterruption];
        if (pause_result < 0) {
            [self clearInterruptionState];
        }
        return;
    }

    if (type != AVAudioSessionInterruptionTypeEnded) {
        return;
    }

    NSNumber *options_value = notification.userInfo[AVAudioSessionInterruptionOptionKey];
    const AVAudioSessionInterruptionOptions options =
        (AVAudioSessionInterruptionOptions)options_value.unsignedIntegerValue;
    const BOOL should_resume =
        (options & AVAudioSessionInterruptionOptionShouldResume) != 0;
    const BOOL can_resume = should_resume &&
                            _was_playing_before_interruption &&
                            _paused_by_interruption &&
                            !_paused_by_route_change;

    [self clearInterruptionState];
    if (!can_resume) {
        return;
    }

    if (self.isManagingAudioSession) {
        NSError *error = nil;
        if (![[HJAudioSession sharedInstance] setActive:YES error:&error]) {
            (void)error;
            return;
        }
    }

    [owner hj_playerAudioSessionControllerResumeForReason:
        HJPlayerAudioSessionResumeReasonInterruptionEnded];
}

- (void)audioSessionRouteChange:(NSNotification *)notification {
    id<HJPlayerAudioSessionControllerOwner> owner = self.owner;
    if (owner == nil) {
        _pending_pause_reason = HJPlayerAudioSessionPauseReasonUnknown;
        _paused_by_route_change = NO;
        return;
    }

    NSNumber *reason_value = notification.userInfo[AVAudioSessionRouteChangeReasonKey];
    const AVAudioSessionRouteChangeReason reason =
        (AVAudioSessionRouteChangeReason)reason_value.unsignedIntegerValue;
    if (reason != AVAudioSessionRouteChangeReasonOldDeviceUnavailable) {
        return;
    }

    if (!self.pausePlayerForAudioSessionEvents) {
        return;
    }

    if (![owner hj_playerAudioSessionControllerIsPlaying]) {
        return;
    }

    _pending_pause_reason = HJPlayerAudioSessionPauseReasonRouteChange;
    const NSInteger pause_result =
        [owner hj_playerAudioSessionControllerPauseForReason:
            HJPlayerAudioSessionPauseReasonRouteChange];
    if (pause_result < 0) {
        _pending_pause_reason = HJPlayerAudioSessionPauseReasonUnknown;
    }
}

- (void)clearInterruptionState {
    _was_playing_before_interruption = NO;
    _paused_by_interruption = NO;
    if (_pending_pause_reason == HJPlayerAudioSessionPauseReasonInterruption) {
        _pending_pause_reason = HJPlayerAudioSessionPauseReasonUnknown;
    }
}

- (void)clearPauseState {
    _was_playing_before_interruption = NO;
    _paused_by_interruption = NO;
    _paused_by_route_change = NO;
    _pending_pause_reason = HJPlayerAudioSessionPauseReasonUnknown;
}

- (void)deactivateAudioSessionIfNeeded {
    if (!self.isManagingAudioSession) {
        return;
    }

    NSError *error = nil;
    [[HJAudioSession sharedInstance] setActive:NO error:&error];
    (void)error;
}

@end
