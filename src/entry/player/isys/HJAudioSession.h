#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, HJAudioSessionCategory) {
    HJAudioSessionCategoryUnknown = -1,
    HJAudioSessionCategoryAmbient = 0,
    HJAudioSessionCategorySoloAmbient = 1,
    HJAudioSessionCategoryPlayback = 2,
    HJAudioSessionCategoryRecord = 3,
    HJAudioSessionCategoryPlayAndRecord = 4,
    HJAudioSessionCategoryMultiRoute = 5,
};

typedef NS_ENUM(NSInteger, HJAudioSessionMode) {
    HJAudioSessionModeUnknown = -1,
    HJAudioSessionModeDefault = 0,
    HJAudioSessionModeVoiceChat = 1,
    HJAudioSessionModeGameChat = 2,
    HJAudioSessionModeVideoRecording = 3,
    HJAudioSessionModeMeasurement = 4,
    HJAudioSessionModeMoviePlayback = 5,
    HJAudioSessionModeVideoChat = 6,
    HJAudioSessionModeSpokenAudio = 7,
    HJAudioSessionModeVoicePrompt = 8,
};

typedef NS_OPTIONS(NSUInteger, HJAudioSessionCategoryOptions) {
    HJAudioSessionCategoryOptionNone = 0,
    HJAudioSessionCategoryOptionMixWithOthers = 0x1,
    HJAudioSessionCategoryOptionDuckOthers = 0x2,
    HJAudioSessionCategoryOptionAllowBluetoothHFP = 0x4,
    HJAudioSessionCategoryOptionDefaultToSpeaker = 0x8,
    HJAudioSessionCategoryOptionInterruptSpokenAudioAndMixWithOthers = 0x11,
    HJAudioSessionCategoryOptionAllowBluetoothA2DP = 0x20,
    HJAudioSessionCategoryOptionAllowAirPlay = 0x40,
    HJAudioSessionCategoryOptionOverrideMutedMicrophoneInterruption = 0x80,
};

typedef NS_ENUM(NSInteger, HJAudioSessionRouteSharingPolicy) {
    HJAudioSessionRouteSharingPolicyUnknown = -1,
    HJAudioSessionRouteSharingPolicyDefault = 0,
    HJAudioSessionRouteSharingPolicyLongFormAudio = 1,
    HJAudioSessionRouteSharingPolicyIndependent = 2,
    HJAudioSessionRouteSharingPolicyLongFormVideo = 3,
};

typedef NS_OPTIONS(NSUInteger, HJAudioSessionSetActiveOptions) {
    HJAudioSessionSetActiveOptionNone = 0,
    HJAudioSessionSetActiveOptionNotifyOthersOnDeactivation = 1 << 0,
};

typedef NS_ENUM(NSInteger, HJAudioSessionPortOverride) {
    HJAudioSessionPortOverrideNone = 0,
    HJAudioSessionPortOverrideSpeaker = 1,
};

typedef NS_ENUM(NSInteger, HJAudioSessionRecordPermission) {
    HJAudioSessionRecordPermissionUnknown = -1,
    HJAudioSessionRecordPermissionUndetermined = 0,
    HJAudioSessionRecordPermissionDenied = 1,
    HJAudioSessionRecordPermissionGranted = 2,
};

FOUNDATION_EXPORT NSString * const HJAudioSessionErrorDomain;

typedef NS_ERROR_ENUM(HJAudioSessionErrorDomain, HJAudioSessionErrorCode) {
    HJAudioSessionErrorCodeInvalidConfiguration = 1,
    HJAudioSessionErrorCodeUnsupportedCategory = 2,
    HJAudioSessionErrorCodeUnsupportedMode = 3,
    HJAudioSessionErrorCodeUnsupportedRouteSharingPolicy = 4,
    HJAudioSessionErrorCodeUnsupportedPortOverride = 5,
    HJAudioSessionErrorCodePreferredInputNotFound = 6,
    HJAudioSessionErrorCodeSpeakerRequiresPlayAndRecord = 7,
    HJAudioSessionErrorCodeUnsupportedCapability = 8,
};

@interface HJAudioSessionConfiguration : NSObject

@property (nonatomic, assign) HJAudioSessionCategory category;
@property (nonatomic, assign) HJAudioSessionMode mode;
@property (nonatomic, assign) HJAudioSessionCategoryOptions categoryOptions;
@property (nonatomic, assign) HJAudioSessionRouteSharingPolicy routeSharingPolicy;
@property (nonatomic, assign) double preferredSampleRate;
@property (nonatomic, assign) NSTimeInterval preferredIOBufferDuration;
@property (nonatomic, assign) NSInteger preferredInputNumberOfChannels;
@property (nonatomic, assign) NSInteger preferredOutputNumberOfChannels;
@property (nonatomic, assign) BOOL preferSpeaker;
@property (nonatomic, assign) BOOL allowHapticsAndSystemSoundsDuringRecording;

@end

@interface HJAudioSessionPortInfo : NSObject

@property (nonatomic, copy, readonly) NSString *uid;
@property (nonatomic, copy, readonly) NSString *portName;
@property (nonatomic, copy, readonly) NSString *portType;
@property (nonatomic, assign, readonly) NSInteger channels;

@end

@interface HJAudioSessionRouteInfo : NSObject

@property (nonatomic, copy, readonly) NSArray<HJAudioSessionPortInfo *> *inputs;
@property (nonatomic, copy, readonly) NSArray<HJAudioSessionPortInfo *> *outputs;

@end

@interface HJAudioSession : NSObject

+ (instancetype)sharedInstance;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

- (BOOL)applyConfiguration:(HJAudioSessionConfiguration *)configuration
                     error:(NSError * _Nullable *)error;
- (BOOL)setActive:(BOOL)active
            error:(NSError * _Nullable *)error;
- (BOOL)setActive:(BOOL)active
          options:(HJAudioSessionSetActiveOptions)options
            error:(NSError * _Nullable *)error;
- (BOOL)setPreferredInputByUID:(nullable NSString *)uid
                         error:(NSError * _Nullable *)error;
- (BOOL)overrideOutputAudioPort:(HJAudioSessionPortOverride)portOverride
                          error:(NSError * _Nullable *)error;
- (void)requestRecordPermission:(void (^ _Nullable)(BOOL granted))completionHandler;

@property (nonatomic, assign, readonly) HJAudioSessionCategory category;
@property (nonatomic, assign, readonly) HJAudioSessionMode mode;
@property (nonatomic, assign, readonly) HJAudioSessionCategoryOptions categoryOptions;
@property (nonatomic, assign, readonly) HJAudioSessionRouteSharingPolicy routeSharingPolicy;
@property (nonatomic, assign, readonly) double sampleRate;
@property (nonatomic, assign, readonly) double preferredSampleRate;
@property (nonatomic, assign, readonly) NSTimeInterval ioBufferDuration;
@property (nonatomic, assign, readonly) NSTimeInterval preferredIOBufferDuration;
@property (nonatomic, assign, readonly) NSTimeInterval inputLatency;
@property (nonatomic, assign, readonly) NSTimeInterval outputLatency;
@property (nonatomic, assign, readonly) NSInteger inputNumberOfChannels;
@property (nonatomic, assign, readonly) NSInteger outputNumberOfChannels;
@property (nonatomic, assign, readonly, getter=isInputAvailable) BOOL inputAvailable;
@property (nonatomic, assign, readonly, getter=isOtherAudioPlaying) BOOL otherAudioPlaying;
@property (nonatomic, assign, readonly) BOOL secondaryAudioShouldBeSilencedHint;
@property (nonatomic, assign, readonly) HJAudioSessionRecordPermission recordPermission;
@property (nonatomic, copy, readonly) NSArray<HJAudioSessionPortInfo *> *availableInputs;
@property (nonatomic, strong, readonly) HJAudioSessionRouteInfo *currentRoute;

@end

NS_ASSUME_NONNULL_END
