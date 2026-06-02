#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, HJMusicPlayerSampleFormat) {
    HJMusicPlayerSampleFormatNone  = -1,
    HJMusicPlayerSampleFormatU8    = 0,   ///< unsigned 8 bits
    HJMusicPlayerSampleFormatS16   = 1,   ///< signed 16 bits
    HJMusicPlayerSampleFormatS32   = 2,   ///< signed 32 bits
    HJMusicPlayerSampleFormatFLT   = 3,   ///< float
    HJMusicPlayerSampleFormatDBL   = 4,   ///< double
    HJMusicPlayerSampleFormatU8P   = 5,   ///< unsigned 8 bits, planar
    HJMusicPlayerSampleFormatS16P  = 6,   ///< signed 16 bits, planar
    HJMusicPlayerSampleFormatS32P  = 7,   ///< signed 32 bits, planar
    HJMusicPlayerSampleFormatFLTP  = 8,   ///< float, planar
    HJMusicPlayerSampleFormatDBLP  = 9,   ///< double, planar
    HJMusicPlayerSampleFormatS64   = 10,  ///< signed 64 bits
    HJMusicPlayerSampleFormatS64P  = 11,  ///< signed 64 bits, planar
};

@class HJMusicPlayer;

/// Option keys for HJMusicPlayer initialization
extern NSString * const HJMusicPlayerOptionRepeats;                 ///< NSNumber (NSInteger), default 0
extern NSString * const HJMusicPlayerOptionPrerollDurationMs;       ///< NSNumber (int64_t), default 120
extern NSString * const HJMusicPlayerOptionPCMCallbackIntervalMs;   ///< NSNumber (int64_t), default 20
extern NSString * const HJMusicPlayerOptionSampleRate;              ///< NSNumber (NSInteger), default 48000
extern NSString * const HJMusicPlayerOptionChannels;                ///< NSNumber (NSInteger), default 2
extern NSString * const HJMusicPlayerOptionSampleFormat;            ///< NSNumber (HJMusicPlayerSampleFormat), default HJMusicPlayerSampleFormatS16
extern NSString * const HJMusicPlayerOptionManageAudioSession;      ///< NSNumber (BOOL), default NO
extern NSString * const HJMusicPlayerOptionPausePlayerForAudioSessionEvents; ///< NSNumber (BOOL), default NO,
                                                                             ///< controls interruption/headphone-unplug auto pause
extern NSString * const HJMusicPlayerOpenOptionStartTime;           ///< NSNumber (int64_t), default -1
extern NSString * const HJMusicPlayerOpenOptionAudioTrackID;        ///< NSNumber (NSInteger), default -1
extern NSString * const HJMusicPlayerOpenOptionAutoPlay;            ///< NSNumber (BOOL), default YES

@interface HJAudioTrackInfo : NSObject

@property (nonatomic, assign) NSInteger trackId;
@property (nonatomic, copy) NSString *displayName;
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *language;
@property (nonatomic, copy) NSString *codecName;
@property (nonatomic, assign) NSInteger channels;
@property (nonatomic, assign) NSInteger sampleRate;
@property (nonatomic, assign, getter=isDefaultTrack) BOOL defaultTrack;
@property (nonatomic, assign, getter=isSelected) BOOL selected;

@end

@interface HJPlayerMediaInfo : NSObject

@property (nonatomic, copy) NSString *url;
@property (nonatomic, assign) int64_t duration;
@property (nonatomic, copy) NSString *audioCodec;
@property (nonatomic, assign) NSInteger sampleRate;
@property (nonatomic, assign) NSInteger channels;
@property (nonatomic, assign) NSInteger trackCount;

@end

@protocol HJMusicPlayerDelegate <NSObject>

@optional
- (void)musicPlayerDidPrepare:(HJMusicPlayer *)player;
- (void)musicPlayerDidPause:(HJMusicPlayer *)player;
- (void)musicPlayerDidResume:(HJMusicPlayer *)player;
- (void)musicPlayerDidStop:(HJMusicPlayer *)player;
- (void)musicPlayerDidReachEOF:(HJMusicPlayer *)player;
- (void)musicPlayer:(HJMusicPlayer *)player didFailWithErrorCode:(NSInteger)errorCode message:(NSString *)message;
- (void)musicPlayer:(HJMusicPlayer *)player didUpdateDuration:(int64_t)duration;
- (void)musicPlayer:(HJMusicPlayer *)player didUpdateAudioTracks:(NSArray<HJAudioTrackInfo *> *)audioTracks;
- (void)musicPlayer:(HJMusicPlayer *)player didUpdateMediaInfo:(HJPlayerMediaInfo *)mediaInfo;
- (void)musicPlayer:(HJMusicPlayer *)player
    didRenderPCMData:(NSData *)pcmData
            channels:(NSInteger)channels
          sampleRate:(NSInteger)sampleRate
        sampleFormat:(HJMusicPlayerSampleFormat)sampleFormat
             trackId:(NSInteger)trackId;

@end

@interface HJMusicPlayer : NSObject

- (instancetype)initWithDelegate:(nullable id<HJMusicPlayerDelegate>)delegate
                         options:(nullable NSDictionary<NSString *, id> *)options;

- (NSInteger)prepare;
- (NSInteger)openURL:(nullable NSString *)url options:(nullable NSDictionary<NSString *, id> *)options;
- (NSInteger)pause;
- (NSInteger)resume;
- (NSInteger)stop;
- (NSInteger)seekToMilliseconds:(int64_t)positionMs;
- (NSInteger)setMute:(BOOL)mute;
- (NSInteger)setVolume:(float)volume;
- (NSInteger)setRepeats:(NSInteger)repeats;
- (int64_t)getCurrentTimestamp;
- (NSInteger)selectAudioTrack:(NSInteger)trackId;
- (void)destroy;

@property (nonatomic, weak, nullable) id<HJMusicPlayerDelegate> delegate;
@property (nonatomic, assign, readonly, getter=isPrepared) BOOL prepared;
@property (nonatomic, assign, readonly, getter=isPaused) BOOL paused;
@property (nonatomic, assign, readonly, getter=isMuted) BOOL muted;
@property (nonatomic, assign, readonly) float currentVolume;
@property (nonatomic, assign, readonly) int64_t duration;
@property (nonatomic, copy, readonly) NSArray<HJAudioTrackInfo *> *audioTracks;
@property (nonatomic, strong, readonly, nullable) HJPlayerMediaInfo *mediaInfo;

@end

NS_ASSUME_NONNULL_END
