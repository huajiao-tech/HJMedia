#import <Foundation/Foundation.h>

#import "HJAudioSession.h"

static int TestSharedInstanceAndQueries(void) {
    HJAudioSession *session = [HJAudioSession sharedInstance];
    if (session == nil) {
        return 1;
    }

    (void)session.sampleRate;
    (void)session.inputAvailable;
    (void)session.currentRoute;
    (void)session.availableInputs;
    return 0;
}

static int TestPlaybackConfigurationAndActivation(void) {
    HJAudioSession *session = [HJAudioSession sharedInstance];
    HJAudioSessionConfiguration *configuration = [[HJAudioSessionConfiguration alloc] init];
    configuration.category = HJAudioSessionCategoryPlayback;
    configuration.mode = HJAudioSessionModeMoviePlayback;
    configuration.categoryOptions = HJAudioSessionCategoryOptionMixWithOthers;
    configuration.preferredSampleRate = 48000.0;
    configuration.preferredIOBufferDuration = 0.02;

    NSError *error = nil;
    if (![session applyConfiguration:configuration error:&error]) {
        return 2;
    }

    if (![session setActive:YES error:&error]) {
        return 3;
    }

    if (![session setActive:NO error:&error]) {
        return 4;
    }

    return 0;
}

static int TestInvalidSpeakerConfiguration(void) {
    HJAudioSession *session = [HJAudioSession sharedInstance];
    HJAudioSessionConfiguration *configuration = [[HJAudioSessionConfiguration alloc] init];
    configuration.category = HJAudioSessionCategoryPlayback;
    configuration.mode = HJAudioSessionModeDefault;
    configuration.preferSpeaker = YES;

    NSError *error = nil;
    if ([session applyConfiguration:configuration error:&error]) {
        return 5;
    }

    if (error == nil) {
        return 6;
    }

    return 0;
}

int main(void) {
    @autoreleasepool {
        const int ret1 = TestSharedInstanceAndQueries();
        if (ret1 != 0) {
            return ret1;
        }

        const int ret2 = TestPlaybackConfigurationAndActivation();
        if (ret2 != 0) {
            return ret2;
        }

        const int ret3 = TestInvalidSpeakerConfiguration();
        if (ret3 != 0) {
            return ret3;
        }

        return 0;
    }
}
