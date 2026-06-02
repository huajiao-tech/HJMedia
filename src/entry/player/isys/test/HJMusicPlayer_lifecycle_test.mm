#import <Foundation/Foundation.h>

#import "HJMusicPlayer.h"
#import "HJPlayerAudioSessionController.h"

@interface HJMusicPlayer (Testing)

@property (nonatomic, strong, readonly) HJPlayerAudioSessionController *audioSessionController;

@end

static int TestManageAudioSessionDefaultsToNo(void) {
    HJMusicPlayer *player = [[HJMusicPlayer alloc] initWithDelegate:nil options:nil];
    if (player == nil) {
        return 1;
    }
    if (player.audioSessionController == nil) {
        return 2;
    }
    if (player.audioSessionController.isManagingAudioSession) {
        return 3;
    }
    if (player.audioSessionController.pausePlayerForAudioSessionEvents) {
        return 4;
    }
    if (!player.audioSessionController.isObserving) {
        return 5;
    }

    [player destroy];
    if (player.audioSessionController.isObserving) {
        return 6;
    }
    return 0;
}

static int TestAudioSessionOptionsCanBeEnabled(void) {
    NSDictionary<NSString *, id> *options = @{
        HJMusicPlayerOptionManageAudioSession : @YES,
        HJMusicPlayerOptionPausePlayerForAudioSessionEvents : @YES,
    };
    HJMusicPlayer *player = [[HJMusicPlayer alloc] initWithDelegate:nil options:options];
    if (player == nil) {
        return 7;
    }
    if (player.audioSessionController == nil) {
        return 8;
    }
    if (!player.audioSessionController.isManagingAudioSession) {
        return 9;
    }
    if (!player.audioSessionController.pausePlayerForAudioSessionEvents) {
        return 10;
    }

    [player destroy];
    if (player.audioSessionController.isObserving) {
        return 11;
    }
    return 0;
}

int main(void) {
    @autoreleasepool {
        const int ret1 = TestManageAudioSessionDefaultsToNo();
        if (ret1 != 0) {
            return ret1;
        }

        const int ret2 = TestAudioSessionOptionsCanBeEnabled();
        if (ret2 != 0) {
            return ret2;
        }

        return 0;
    }
}
