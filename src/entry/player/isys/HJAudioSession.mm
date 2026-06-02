#import "HJAudioSession.h"

#import <AVFAudio/AVFAudio.h>

NSString * const HJAudioSessionErrorDomain = @"com.hjmedia.ios.audio_session";

static const void *kHJAudioSessionQueueKey = &kHJAudioSessionQueueKey;
static const HJAudioSessionCategoryOptions kHJAudioSessionSupportedCategoryOptionsMask =
    HJAudioSessionCategoryOptionMixWithOthers |
    HJAudioSessionCategoryOptionDuckOthers |
    HJAudioSessionCategoryOptionAllowBluetoothHFP |
    HJAudioSessionCategoryOptionDefaultToSpeaker |
    HJAudioSessionCategoryOptionInterruptSpokenAudioAndMixWithOthers |
    HJAudioSessionCategoryOptionAllowBluetoothA2DP |
    HJAudioSessionCategoryOptionAllowAirPlay |
    HJAudioSessionCategoryOptionOverrideMutedMicrophoneInterruption;

static NSError *HJAudioSessionMakeError(HJAudioSessionErrorCode code, NSString *description) {
    return [NSError errorWithDomain:HJAudioSessionErrorDomain
                               code:code
                           userInfo:@{NSLocalizedDescriptionKey : description ?: @""}];
}

static AVAudioSessionCategory HJAudioSessionCategoryToAVCategory(HJAudioSessionCategory category) {
    switch (category) {
        case HJAudioSessionCategoryAmbient:
            return AVAudioSessionCategoryAmbient;
        case HJAudioSessionCategorySoloAmbient:
            return AVAudioSessionCategorySoloAmbient;
        case HJAudioSessionCategoryPlayback:
            return AVAudioSessionCategoryPlayback;
        case HJAudioSessionCategoryRecord:
            return AVAudioSessionCategoryRecord;
        case HJAudioSessionCategoryPlayAndRecord:
            return AVAudioSessionCategoryPlayAndRecord;
        case HJAudioSessionCategoryMultiRoute:
            return AVAudioSessionCategoryMultiRoute;
        case HJAudioSessionCategoryUnknown:
        default:
            return nil;
    }
}

static HJAudioSessionCategory HJAudioSessionCategoryFromAVCategory(AVAudioSessionCategory category) {
    if ([category isEqualToString:AVAudioSessionCategoryAmbient]) {
        return HJAudioSessionCategoryAmbient;
    }
    if ([category isEqualToString:AVAudioSessionCategorySoloAmbient]) {
        return HJAudioSessionCategorySoloAmbient;
    }
    if ([category isEqualToString:AVAudioSessionCategoryPlayback]) {
        return HJAudioSessionCategoryPlayback;
    }
    if ([category isEqualToString:AVAudioSessionCategoryRecord]) {
        return HJAudioSessionCategoryRecord;
    }
    if ([category isEqualToString:AVAudioSessionCategoryPlayAndRecord]) {
        return HJAudioSessionCategoryPlayAndRecord;
    }
    if ([category isEqualToString:AVAudioSessionCategoryMultiRoute]) {
        return HJAudioSessionCategoryMultiRoute;
    }
    return HJAudioSessionCategoryUnknown;
}

static AVAudioSessionMode HJAudioSessionModeToAVMode(HJAudioSessionMode mode) {
    switch (mode) {
        case HJAudioSessionModeDefault:
            return AVAudioSessionModeDefault;
        case HJAudioSessionModeVoiceChat:
            return AVAudioSessionModeVoiceChat;
        case HJAudioSessionModeGameChat:
            return AVAudioSessionModeGameChat;
        case HJAudioSessionModeVideoRecording:
            return AVAudioSessionModeVideoRecording;
        case HJAudioSessionModeMeasurement:
            return AVAudioSessionModeMeasurement;
        case HJAudioSessionModeMoviePlayback:
            return AVAudioSessionModeMoviePlayback;
        case HJAudioSessionModeVideoChat:
            return AVAudioSessionModeVideoChat;
        case HJAudioSessionModeSpokenAudio:
            return AVAudioSessionModeSpokenAudio;
        case HJAudioSessionModeVoicePrompt:
            return AVAudioSessionModeVoicePrompt;
        case HJAudioSessionModeUnknown:
        default:
            return nil;
    }
}

static HJAudioSessionMode HJAudioSessionModeFromAVMode(AVAudioSessionMode mode) {
    if ([mode isEqualToString:AVAudioSessionModeDefault]) {
        return HJAudioSessionModeDefault;
    }
    if ([mode isEqualToString:AVAudioSessionModeVoiceChat]) {
        return HJAudioSessionModeVoiceChat;
    }
    if ([mode isEqualToString:AVAudioSessionModeGameChat]) {
        return HJAudioSessionModeGameChat;
    }
    if ([mode isEqualToString:AVAudioSessionModeVideoRecording]) {
        return HJAudioSessionModeVideoRecording;
    }
    if ([mode isEqualToString:AVAudioSessionModeMeasurement]) {
        return HJAudioSessionModeMeasurement;
    }
    if ([mode isEqualToString:AVAudioSessionModeMoviePlayback]) {
        return HJAudioSessionModeMoviePlayback;
    }
    if ([mode isEqualToString:AVAudioSessionModeVideoChat]) {
        return HJAudioSessionModeVideoChat;
    }
    if ([mode isEqualToString:AVAudioSessionModeSpokenAudio]) {
        return HJAudioSessionModeSpokenAudio;
    }
    if ([mode isEqualToString:AVAudioSessionModeVoicePrompt]) {
        return HJAudioSessionModeVoicePrompt;
    }
    return HJAudioSessionModeUnknown;
}

static AVAudioSessionCategoryOptions HJAudioSessionCategoryOptionsToAVOptions(
    HJAudioSessionCategoryOptions options) {
    return (AVAudioSessionCategoryOptions)(options & kHJAudioSessionSupportedCategoryOptionsMask);
}

static HJAudioSessionCategoryOptions HJAudioSessionCategoryOptionsFromAVOptions(
    AVAudioSessionCategoryOptions options) {
    return (HJAudioSessionCategoryOptions)(options & kHJAudioSessionSupportedCategoryOptionsMask);
}

static AVAudioSessionRouteSharingPolicy HJAudioSessionRouteSharingPolicyToAVPolicy(
    HJAudioSessionRouteSharingPolicy policy) {
    switch (policy) {
        case HJAudioSessionRouteSharingPolicyDefault:
            return AVAudioSessionRouteSharingPolicyDefault;
        case HJAudioSessionRouteSharingPolicyLongFormAudio:
            return AVAudioSessionRouteSharingPolicyLongFormAudio;
        case HJAudioSessionRouteSharingPolicyIndependent:
            return AVAudioSessionRouteSharingPolicyIndependent;
        case HJAudioSessionRouteSharingPolicyLongFormVideo:
            if (@available(iOS 13.0, *)) {
                return AVAudioSessionRouteSharingPolicyLongFormVideo;
            }
            return AVAudioSessionRouteSharingPolicyDefault;
        case HJAudioSessionRouteSharingPolicyUnknown:
        default:
            return AVAudioSessionRouteSharingPolicyDefault;
    }
}

static HJAudioSessionRouteSharingPolicy HJAudioSessionRouteSharingPolicyFromAVPolicy(
    AVAudioSessionRouteSharingPolicy policy) {
    switch (policy) {
        case AVAudioSessionRouteSharingPolicyDefault:
            return HJAudioSessionRouteSharingPolicyDefault;
        case AVAudioSessionRouteSharingPolicyLongFormAudio:
            return HJAudioSessionRouteSharingPolicyLongFormAudio;
        case AVAudioSessionRouteSharingPolicyIndependent:
            return HJAudioSessionRouteSharingPolicyIndependent;
        case AVAudioSessionRouteSharingPolicyLongFormVideo:
            return HJAudioSessionRouteSharingPolicyLongFormVideo;
        default:
            return HJAudioSessionRouteSharingPolicyUnknown;
    }
}

static AVAudioSessionSetActiveOptions HJAudioSessionSetActiveOptionsToAVOptions(
    HJAudioSessionSetActiveOptions options) {
    return (AVAudioSessionSetActiveOptions)options;
}

static AVAudioSessionPortOverride HJAudioSessionPortOverrideToAVOverride(
    HJAudioSessionPortOverride port_override) {
    switch (port_override) {
        case HJAudioSessionPortOverrideNone:
            return AVAudioSessionPortOverrideNone;
        case HJAudioSessionPortOverrideSpeaker:
            return AVAudioSessionPortOverrideSpeaker;
        default:
            return AVAudioSessionPortOverrideNone;
    }
}

static BOOL HJAudioSessionPolicyNeedsPlaybackConstraints(HJAudioSessionRouteSharingPolicy policy) {
    return policy == HJAudioSessionRouteSharingPolicyLongFormAudio ||
           policy == HJAudioSessionRouteSharingPolicyLongFormVideo;
}

@interface HJAudioSessionConfiguration ()

@end

@implementation HJAudioSessionConfiguration

- (instancetype)init {
    self = [super init];
    if (self) {
        _category = HJAudioSessionCategoryPlayback;
        _mode = HJAudioSessionModeDefault;
        _categoryOptions = HJAudioSessionCategoryOptionNone;
        _routeSharingPolicy = HJAudioSessionRouteSharingPolicyDefault;
        _preferredSampleRate = 0.0;
        _preferredIOBufferDuration = 0.0;
        _preferredInputNumberOfChannels = 0;
        _preferredOutputNumberOfChannels = 0;
        _preferSpeaker = NO;
        _allowHapticsAndSystemSoundsDuringRecording = NO;
    }
    return self;
}

@end

@interface HJAudioSessionPortInfo ()

@property (nonatomic, copy, readwrite) NSString *uid;
@property (nonatomic, copy, readwrite) NSString *portName;
@property (nonatomic, copy, readwrite) NSString *portType;
@property (nonatomic, assign, readwrite) NSInteger channels;

@end

@implementation HJAudioSessionPortInfo

@end

@interface HJAudioSessionRouteInfo ()

@property (nonatomic, copy, readwrite) NSArray<HJAudioSessionPortInfo *> *inputs;
@property (nonatomic, copy, readwrite) NSArray<HJAudioSessionPortInfo *> *outputs;

@end

@implementation HJAudioSessionRouteInfo

@end

@interface HJAudioSession () {
    dispatch_queue_t _session_queue;
}

- (instancetype)initPrivate;
- (BOOL)performMutationWithError:(NSError * _Nullable *)error
                           block:(BOOL (^)(AVAudioSession *session, NSError **mutationError))block;
- (NSError *)validateConfiguration:(HJAudioSessionConfiguration *)configuration;
- (BOOL)applySpeakerPreference:(BOOL)prefer_speaker
                       session:(AVAudioSession *)session
                         error:(NSError **)error;

@end

@implementation HJAudioSession

+ (instancetype)sharedInstance {
    static HJAudioSession *shared_session = nil;
    static dispatch_once_t once_token;
    dispatch_once(&once_token, ^{
        shared_session = [[HJAudioSession alloc] initPrivate];
    });
    return shared_session;
}

- (instancetype)initPrivate {
    self = [super init];
    if (self) {
        _session_queue = dispatch_queue_create("com.hjmedia.audio_session.serial",
                                               DISPATCH_QUEUE_SERIAL);
        dispatch_queue_set_specific(_session_queue,
                                    kHJAudioSessionQueueKey,
                                    (void *)kHJAudioSessionQueueKey,
                                    NULL);
    }
    return self;
}

- (BOOL)performMutationWithError:(NSError * _Nullable *)error
                           block:(BOOL (^)(AVAudioSession *session, NSError **mutationError))block {
    if (block == nil) {
        if (error != NULL) {
            *error = HJAudioSessionMakeError(HJAudioSessionErrorCodeInvalidConfiguration,
                                             @"Audio session operation block is nil.");
        }
        return NO;
    }

    __block BOOL success = NO;
    __block NSError *mutation_error = nil;
    void (^work)(void) = ^{
        AVAudioSession *session = [AVAudioSession sharedInstance];
        success = block(session, &mutation_error);
    };

    if (dispatch_get_specific(kHJAudioSessionQueueKey) != NULL) {
        work();
    } else {
        dispatch_sync(_session_queue, work);
    }

    if (error != NULL) {
        *error = success ? nil : mutation_error;
    }
    return success;
}

- (NSError *)validateConfiguration:(HJAudioSessionConfiguration *)configuration {
    if (configuration == nil) {
        return HJAudioSessionMakeError(HJAudioSessionErrorCodeInvalidConfiguration,
                                       @"Audio session configuration must not be nil.");
    }

    if (HJAudioSessionCategoryToAVCategory(configuration.category) == nil) {
        return HJAudioSessionMakeError(HJAudioSessionErrorCodeUnsupportedCategory,
                                       @"Unsupported audio session category.");
    }

    if (HJAudioSessionModeToAVMode(configuration.mode) == nil) {
        return HJAudioSessionMakeError(HJAudioSessionErrorCodeUnsupportedMode,
                                       @"Unsupported audio session mode.");
    }

    if (configuration.routeSharingPolicy == HJAudioSessionRouteSharingPolicyUnknown) {
        return HJAudioSessionMakeError(HJAudioSessionErrorCodeUnsupportedRouteSharingPolicy,
                                       @"Unsupported route sharing policy.");
    }

    if (configuration.preferSpeaker &&
        configuration.category != HJAudioSessionCategoryPlayAndRecord) {
        return HJAudioSessionMakeError(HJAudioSessionErrorCodeSpeakerRequiresPlayAndRecord,
                                       @"preferSpeaker requires play-and-record category.");
    }

    if (HJAudioSessionPolicyNeedsPlaybackConstraints(configuration.routeSharingPolicy)) {
        const BOOL valid_category = configuration.category == HJAudioSessionCategoryPlayback;
        const BOOL valid_mode =
            configuration.mode == HJAudioSessionModeDefault ||
            configuration.mode == HJAudioSessionModeMoviePlayback ||
            configuration.mode == HJAudioSessionModeSpokenAudio;
        const BOOL no_options =
            configuration.categoryOptions == HJAudioSessionCategoryOptionNone;
        if (!valid_category || !valid_mode || !no_options) {
            return HJAudioSessionMakeError(HJAudioSessionErrorCodeInvalidConfiguration,
                                           @"Long-form route sharing requires playback category, default/movie/spoken mode, and no category options.");
        }
    }

    if (configuration.routeSharingPolicy == HJAudioSessionRouteSharingPolicyLongFormVideo) {
        if (@available(iOS 13.0, *)) {
        } else {
            return HJAudioSessionMakeError(HJAudioSessionErrorCodeUnsupportedRouteSharingPolicy,
                                           @"Long-form video route sharing requires iOS 13.0 or later.");
        }
    }

    if (configuration.allowHapticsAndSystemSoundsDuringRecording) {
        if (@available(iOS 13.0, *)) {
        } else {
            return HJAudioSessionMakeError(HJAudioSessionErrorCodeUnsupportedCapability,
                                           @"Haptics during recording requires iOS 13.0 or later.");
        }
    }

    return nil;
}

- (BOOL)applySpeakerPreference:(BOOL)prefer_speaker
                       session:(AVAudioSession *)session
                         error:(NSError **)error {
    if (HJAudioSessionCategoryFromAVCategory(session.category) != HJAudioSessionCategoryPlayAndRecord) {
        return YES;
    }

    const AVAudioSessionPortOverride port_override =
        prefer_speaker ? AVAudioSessionPortOverrideSpeaker : AVAudioSessionPortOverrideNone;
    return [session overrideOutputAudioPort:port_override error:error];
}

- (BOOL)applyConfiguration:(HJAudioSessionConfiguration *)configuration
                     error:(NSError * _Nullable *)error {
    NSError *validation_error = [self validateConfiguration:configuration];
    if (validation_error != nil) {
        if (error != NULL) {
            *error = validation_error;
        }
        return NO;
    }

    return [self performMutationWithError:error
                                    block:^BOOL(AVAudioSession *session, NSError **mutationError) {
        const AVAudioSessionCategory category =
            HJAudioSessionCategoryToAVCategory(configuration.category);
        const AVAudioSessionMode mode = HJAudioSessionModeToAVMode(configuration.mode);
        const AVAudioSessionCategoryOptions options =
            HJAudioSessionCategoryOptionsToAVOptions(configuration.categoryOptions);

        BOOL success = NO;
        if (@available(iOS 11.0, *)) {
            const AVAudioSessionRouteSharingPolicy policy =
                HJAudioSessionRouteSharingPolicyToAVPolicy(configuration.routeSharingPolicy);
            success = [session setCategory:category
                                      mode:mode
                        routeSharingPolicy:policy
                                   options:options
                                     error:mutationError];
        } else {
            success = [session setCategory:category
                                      mode:mode
                                   options:options
                                     error:mutationError];
        }
        if (!success) {
            return NO;
        }

        if (configuration.preferredSampleRate > 0.0 &&
            ![session setPreferredSampleRate:configuration.preferredSampleRate
                                       error:mutationError]) {
            return NO;
        }

        if (configuration.preferredIOBufferDuration > 0.0 &&
            ![session setPreferredIOBufferDuration:configuration.preferredIOBufferDuration
                                             error:mutationError]) {
            return NO;
        }

        if (configuration.preferredInputNumberOfChannels > 0 &&
            ![session setPreferredInputNumberOfChannels:configuration.preferredInputNumberOfChannels
                                                  error:mutationError]) {
            return NO;
        }

        if (configuration.preferredOutputNumberOfChannels > 0 &&
            ![session setPreferredOutputNumberOfChannels:configuration.preferredOutputNumberOfChannels
                                                   error:mutationError]) {
            return NO;
        }

        if (@available(iOS 13.0, *)) {
            if (![session setAllowHapticsAndSystemSoundsDuringRecording:
                             configuration.allowHapticsAndSystemSoundsDuringRecording
                                                           error:mutationError]) {
                return NO;
            }
        }

        return [self applySpeakerPreference:configuration.preferSpeaker
                                    session:session
                                      error:mutationError];
    }];
}

- (BOOL)setActive:(BOOL)active
            error:(NSError * _Nullable *)error {
    return [self setActive:active
                   options:HJAudioSessionSetActiveOptionNone
                     error:error];
}

- (BOOL)setActive:(BOOL)active
          options:(HJAudioSessionSetActiveOptions)options
            error:(NSError * _Nullable *)error {
    return [self performMutationWithError:error
                                    block:^BOOL(AVAudioSession *session, NSError **mutationError) {
        return [session setActive:active
                      withOptions:HJAudioSessionSetActiveOptionsToAVOptions(options)
                            error:mutationError];
    }];
}

- (BOOL)setPreferredInputByUID:(nullable NSString *)uid
                         error:(NSError * _Nullable *)error {
    return [self performMutationWithError:error
                                    block:^BOOL(AVAudioSession *session, NSError **mutationError) {
        if (uid.length == 0) {
            return [session setPreferredInput:nil error:mutationError];
        }

        AVAudioSessionPortDescription *matched_input = nil;
        for (AVAudioSessionPortDescription *input in session.availableInputs ?: @[]) {
            if ([input.UID isEqualToString:uid]) {
                matched_input = input;
                break;
            }
        }

        if (matched_input == nil) {
            if (mutationError != NULL) {
                *mutationError = HJAudioSessionMakeError(HJAudioSessionErrorCodePreferredInputNotFound,
                                                         @"Preferred input UID was not found in available inputs.");
            }
            return NO;
        }

        return [session setPreferredInput:matched_input error:mutationError];
    }];
}

- (BOOL)overrideOutputAudioPort:(HJAudioSessionPortOverride)portOverride
                          error:(NSError * _Nullable *)error {
    if (portOverride != HJAudioSessionPortOverrideNone &&
        portOverride != HJAudioSessionPortOverrideSpeaker) {
        if (error != NULL) {
            *error = HJAudioSessionMakeError(HJAudioSessionErrorCodeUnsupportedPortOverride,
                                             @"Unsupported output audio port override.");
        }
        return NO;
    }

    return [self performMutationWithError:error
                                    block:^BOOL(AVAudioSession *session, NSError **mutationError) {
        if (HJAudioSessionCategoryFromAVCategory(session.category) !=
            HJAudioSessionCategoryPlayAndRecord) {
            if (mutationError != NULL) {
                *mutationError = HJAudioSessionMakeError(HJAudioSessionErrorCodeSpeakerRequiresPlayAndRecord,
                                                         @"Output audio port override requires play-and-record category.");
            }
            return NO;
        }

        return [session overrideOutputAudioPort:HJAudioSessionPortOverrideToAVOverride(portOverride)
                                          error:mutationError];
    }];
}

- (void)requestRecordPermission:(void (^ _Nullable)(BOOL granted))completionHandler {
    void (^safe_handler)(BOOL granted) = completionHandler ?: ^(BOOL granted) {
        (void)granted;
    };

    if (@available(iOS 17.0, *)) {
        [AVAudioApplication requestRecordPermissionWithCompletionHandler:safe_handler];
        return;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [[AVAudioSession sharedInstance] requestRecordPermission:safe_handler];
#pragma clang diagnostic pop
}

- (HJAudioSessionCategory)category {
    return HJAudioSessionCategoryFromAVCategory([AVAudioSession sharedInstance].category);
}

- (HJAudioSessionMode)mode {
    return HJAudioSessionModeFromAVMode([AVAudioSession sharedInstance].mode);
}

- (HJAudioSessionCategoryOptions)categoryOptions {
    return HJAudioSessionCategoryOptionsFromAVOptions([AVAudioSession sharedInstance].categoryOptions);
}

- (HJAudioSessionRouteSharingPolicy)routeSharingPolicy {
    if (@available(iOS 11.0, *)) {
        return HJAudioSessionRouteSharingPolicyFromAVPolicy([AVAudioSession sharedInstance].routeSharingPolicy);
    }
    return HJAudioSessionRouteSharingPolicyDefault;
}

- (double)sampleRate {
    return [AVAudioSession sharedInstance].sampleRate;
}

- (double)preferredSampleRate {
    return [AVAudioSession sharedInstance].preferredSampleRate;
}

- (NSTimeInterval)ioBufferDuration {
    return [AVAudioSession sharedInstance].IOBufferDuration;
}

- (NSTimeInterval)preferredIOBufferDuration {
    return [AVAudioSession sharedInstance].preferredIOBufferDuration;
}

- (NSTimeInterval)inputLatency {
    return [AVAudioSession sharedInstance].inputLatency;
}

- (NSTimeInterval)outputLatency {
    return [AVAudioSession sharedInstance].outputLatency;
}

- (NSInteger)inputNumberOfChannels {
    return [AVAudioSession sharedInstance].inputNumberOfChannels;
}

- (NSInteger)outputNumberOfChannels {
    return [AVAudioSession sharedInstance].outputNumberOfChannels;
}

- (BOOL)isInputAvailable {
    return [AVAudioSession sharedInstance].inputAvailable;
}

- (BOOL)isOtherAudioPlaying {
    return [AVAudioSession sharedInstance].otherAudioPlaying;
}

- (BOOL)secondaryAudioShouldBeSilencedHint {
    return [AVAudioSession sharedInstance].secondaryAudioShouldBeSilencedHint;
}

- (HJAudioSessionRecordPermission)recordPermission {
    if (@available(iOS 17.0, *)) {
        switch (AVAudioApplication.sharedInstance.recordPermission) {
            case AVAudioApplicationRecordPermissionUndetermined:
                return HJAudioSessionRecordPermissionUndetermined;
            case AVAudioApplicationRecordPermissionDenied:
                return HJAudioSessionRecordPermissionDenied;
            case AVAudioApplicationRecordPermissionGranted:
                return HJAudioSessionRecordPermissionGranted;
        }
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    switch ([AVAudioSession sharedInstance].recordPermission) {
        case AVAudioSessionRecordPermissionUndetermined:
            return HJAudioSessionRecordPermissionUndetermined;
        case AVAudioSessionRecordPermissionDenied:
            return HJAudioSessionRecordPermissionDenied;
        case AVAudioSessionRecordPermissionGranted:
            return HJAudioSessionRecordPermissionGranted;
    }
#pragma clang diagnostic pop

    return HJAudioSessionRecordPermissionUnknown;
}

- (HJAudioSessionPortInfo *)portInfoFromDescription:(AVAudioSessionPortDescription *)description {
    HJAudioSessionPortInfo *port_info = [[HJAudioSessionPortInfo alloc] init];
    port_info.uid = description.UID ?: @"";
    port_info.portName = description.portName ?: @"";
    port_info.portType = description.portType ?: @"";
    port_info.channels = description.channels.count;
    return port_info;
}

- (NSArray<HJAudioSessionPortInfo *> *)portInfosFromDescriptions:
    (NSArray<AVAudioSessionPortDescription *> *)descriptions {
    NSMutableArray<HJAudioSessionPortInfo *> *port_infos =
        [NSMutableArray arrayWithCapacity:descriptions.count];
    for (AVAudioSessionPortDescription *description in descriptions) {
        [port_infos addObject:[self portInfoFromDescription:description]];
    }
    return port_infos.copy;
}

- (NSArray<HJAudioSessionPortInfo *> *)availableInputs {
    NSArray<AVAudioSessionPortDescription *> *inputs =
        [AVAudioSession sharedInstance].availableInputs ?: @[];
    return [self portInfosFromDescriptions:inputs];
}

- (HJAudioSessionRouteInfo *)currentRoute {
    AVAudioSessionRouteDescription *route = [AVAudioSession sharedInstance].currentRoute;
    HJAudioSessionRouteInfo *route_info = [[HJAudioSessionRouteInfo alloc] init];
    route_info.inputs = [self portInfosFromDescriptions:route.inputs ?: @[]];
    route_info.outputs = [self portInfosFromDescriptions:route.outputs ?: @[]];
    return route_info;
}

@end
