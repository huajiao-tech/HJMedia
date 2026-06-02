#import "HJMusicPlayer.h"

/// Set to 1 to dispatch all delegate callbacks to the main thread.
/// Set to 0 to call delegate callbacks directly on the engine callback thread (lower latency).
#ifndef HJ_MUSIC_PLAYER_CALLBACK_ON_MAIN_THREAD
#define HJ_MUSIC_PLAYER_CALLBACK_ON_MAIN_THREAD 0
#endif

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>

#include "HJError.h"
#include "HJGraph.h"
#include "HJGraphInfo.h"
#include "HJMediaInfo.h"
#include "HJPlayerContext.h"
#include "HJFLog.h"
#import "HJPlayerAudioSessionController.h"

NS_HJ_USING;

namespace {

struct HJMusicPlayerHandle {
    std::mutex mutex;
    HJGraphPlayer::Ptr player = nullptr;
};

std::string HJNSStringToStdString(NSString *value) {
    if (value == nil || value.length == 0) {
        return std::string();
    }
    return std::string(value.UTF8String);
}

float HJClampVolume(float volume) {
    if (std::isnan(volume)) {
        return 1.0f;
    }
    return std::max(0.0f, std::min(1.0f, volume));
}

}  // namespace

@implementation HJAudioTrackInfo
@end

@implementation HJPlayerMediaInfo
@end

static BOOL HJNullableStringEqual(NSString *lhs, NSString *rhs) {
    if (lhs == rhs) {
        return YES;
    }
    if (lhs == nil || rhs == nil) {
        return NO;
    }
    return [lhs isEqualToString:rhs];
}

static BOOL HJAudioTrackInfoEqual(HJAudioTrackInfo *lhs, HJAudioTrackInfo *rhs) {
    if (lhs == rhs) {
        return YES;
    }
    if (lhs == nil || rhs == nil) {
        return NO;
    }
    return lhs.trackId == rhs.trackId &&
           HJNullableStringEqual(lhs.displayName, rhs.displayName) &&
           HJNullableStringEqual(lhs.title, rhs.title) &&
           HJNullableStringEqual(lhs.language, rhs.language) &&
           HJNullableStringEqual(lhs.codecName, rhs.codecName) &&
           lhs.channels == rhs.channels &&
           lhs.sampleRate == rhs.sampleRate &&
           lhs.isDefaultTrack == rhs.isDefaultTrack &&
           lhs.isSelected == rhs.isSelected;
}

static BOOL HJAudioTrackInfoArrayEqual(NSArray<HJAudioTrackInfo *> *lhs,
                                       NSArray<HJAudioTrackInfo *> *rhs) {
    if (lhs == rhs) {
        return YES;
    }
    if (lhs.count != rhs.count) {
        return NO;
    }
    for (NSUInteger index = 0; index < lhs.count; ++index) {
        if (!HJAudioTrackInfoEqual(lhs[index], rhs[index])) {
            return NO;
        }
    }
    return YES;
}

static BOOL HJPlayerMediaInfoEqual(HJPlayerMediaInfo *lhs, HJPlayerMediaInfo *rhs) {
    if (lhs == rhs) {
        return YES;
    }
    if (lhs == nil || rhs == nil) {
        return NO;
    }
    return HJNullableStringEqual(lhs.url, rhs.url) &&
           lhs.duration == rhs.duration &&
           HJNullableStringEqual(lhs.audioCodec, rhs.audioCodec) &&
           lhs.sampleRate == rhs.sampleRate &&
           lhs.channels == rhs.channels &&
           lhs.trackCount == rhs.trackCount;
}

NSString * const HJMusicPlayerOptionRepeats               = @"repeats";
NSString * const HJMusicPlayerOptionPrerollDurationMs     = @"prerollDurationMs";
NSString * const HJMusicPlayerOptionPCMCallbackIntervalMs = @"pcmCallbackIntervalMs";
NSString * const HJMusicPlayerOptionSampleRate            = @"sampleRate";
NSString * const HJMusicPlayerOptionChannels              = @"channels";
NSString * const HJMusicPlayerOptionSampleFormat          = @"sampleFormat";
NSString * const HJMusicPlayerOptionManageAudioSession    = @"manageAudioSession";
NSString * const HJMusicPlayerOptionPausePlayerForAudioSessionEvents =
    @"pausePlayerForAudioSessionEvents";
NSString * const HJMusicPlayerOpenOptionStartTime         = @"startTime";
NSString * const HJMusicPlayerOpenOptionAudioTrackID      = @"audioTrackID";
NSString * const HJMusicPlayerOpenOptionAutoPlay          = @"autoPlay";

@interface HJMusicPlayer () <HJPlayerAudioSessionControllerOwner>

@property (nonatomic, assign, readwrite, getter=isPrepared) BOOL prepared;
@property (nonatomic, assign, readwrite, getter=isPaused) BOOL paused;
@property (nonatomic, assign, readwrite, getter=isMuted) BOOL muted;
@property (nonatomic, assign, readwrite) float currentVolume;
@property (nonatomic, assign, readwrite) int64_t duration;
@property (nonatomic, copy, readwrite) NSArray<HJAudioTrackInfo *> *audioTracks;
@property (nonatomic, strong, readwrite, nullable) HJPlayerMediaInfo *mediaInfo;
@property (nonatomic, copy, nullable) NSString *currentURL;
@property (nonatomic, strong) HJPlayerAudioSessionController *audioSessionController;

- (NSInteger)hj_prepareLocked:(BOOL *)didCreatePlayer;
- (NSInteger)hj_prepareAudioSessionIfNeeded;
- (HJGraphPlayer::Ptr)hj_detachPlayerLocked;
- (void)hj_resetStateLocked;
- (void)hj_refreshDurationLockedWithFallback:(int64_t)fallbackDuration;
- (void)hj_refreshAudioTracksLocked;
- (void)hj_refreshMediaInfoLocked;
- (BOOL)hj_updateSelectedAudioTrackLocked:(NSInteger)trackId;
- (nullable NSString *)hj_resolvedURL:(nullable NSString *)url;
- (void)hj_notifyOnMainThread:(dispatch_block_t)block;
- (void)hj_dispatchAsyncOnMainThread:(dispatch_block_t)block;
- (void)hj_notifyErrorCode:(NSInteger)errorCode message:(NSString *)message;

@end

@implementation HJMusicPlayer {
    std::shared_ptr<HJMusicPlayerHandle> _handle;
    NSDictionary<NSString *, id> *_options;
    NSInteger _repeats;
}

- (instancetype)initWithDelegate:(id<HJMusicPlayerDelegate>)delegate
                         options:(NSDictionary<NSString *, id> *)options {
    HJFLogi("HJMusicPlayer initWithDelegate options_count:{}", (unsigned long)options.count);
    self = [super init];
    if (self) {
        _handle = std::make_shared<HJMusicPlayerHandle>();
        _delegate = delegate;
        _options = [options copy];
        _prepared = NO;
        _paused = NO;
        _muted = NO;
        _currentVolume = 1.0f;
        _duration = 0;
        _audioTracks = @[];
        _repeats = _options[HJMusicPlayerOptionRepeats]
            ? [_options[HJMusicPlayerOptionRepeats] integerValue]
            : 0;

        const BOOL manage_audio_session = _options[HJMusicPlayerOptionManageAudioSession]
            ? [_options[HJMusicPlayerOptionManageAudioSession] boolValue]
            : NO;
        const BOOL pause_player_for_audio_session_events =
            _options[HJMusicPlayerOptionPausePlayerForAudioSessionEvents]
                ? [_options[HJMusicPlayerOptionPausePlayerForAudioSessionEvents] boolValue]
                : NO;
        _audioSessionController =
            [[HJPlayerAudioSessionController alloc] initWithOwner:self
                                               manageAudioSession:manage_audio_session
                              pausePlayerForAudioSessionEvents:
                                  pause_player_for_audio_session_events];
        [_audioSessionController startObserving];
    }
    return self;
}

- (void)dealloc {
    [self.audioSessionController handlePlayerDidDestroy];
}

- (NSInteger)prepare {
    HJFLogi("HJMusicPlayer prepare");
    const NSInteger audio_session_result = [self hj_prepareAudioSessionIfNeeded];
    if (audio_session_result < 0) {
        HJFLoge("HJMusicPlayer prepare failed: audio_session_result={}", audio_session_result);
        return audio_session_result;
    }

    HJGraphPlayer::Ptr player_to_release = nullptr;
    BOOL did_create_player = NO;
    NSInteger prepare_result = HJ_OK;
    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        if (!(self.prepared && _handle->player)) {
            if (_handle->player != nullptr) {
                player_to_release = [self hj_detachPlayerLocked];
            }
        }
    }
    if (player_to_release) {
        player_to_release->done();
    }

    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        prepare_result = [self hj_prepareLocked:&did_create_player];
    }
    if (prepare_result < 0) {
        HJFLoge("HJMusicPlayer prepare failed: prepare_result={}", prepare_result);
        return prepare_result;
    }

    if (did_create_player) {
        [self hj_notifyOnMainThread:^{
            id<HJMusicPlayerDelegate> delegate = self.delegate;
            if ([delegate respondsToSelector:@selector(musicPlayerDidPrepare:)]) {
                [delegate musicPlayerDidPrepare:self];
            }
        }];
    }
    return prepare_result;
}

- (NSInteger)openURL:(NSString *)url options:(NSDictionary<NSString *, id> *)options
{
    HJFLogi("HJMusicPlayer openURL: {}", url ? url.UTF8String : "null");
    NSString *resolvedURL = [self hj_resolvedURL:url];
    if (resolvedURL.length == 0) {
        HJFLoge("HJMusicPlayer openURL failed: resolved URL is empty, input_url={}",
                HJNSStringToStdString(url));
        return HJErrInvalidParams;
    }
    int64_t startTime = -1;
    int audioTrackID = -1;
    BOOL autoPlay = YES;
    if (options) {
        id startTimeObject = options[HJMusicPlayerOpenOptionStartTime];
        if (startTimeObject && [startTimeObject isKindOfClass:[NSNumber class]]) {
            startTime = [startTimeObject longLongValue];
        }
        id trackIDObject = options[HJMusicPlayerOpenOptionAudioTrackID];
        if (trackIDObject && [trackIDObject isKindOfClass:[NSNumber class]]) {
            audioTrackID = [trackIDObject intValue];
        }
        id autoPlayObject = options[HJMusicPlayerOpenOptionAutoPlay];
        if (autoPlayObject && [autoPlayObject isKindOfClass:[NSNumber class]]) {
            autoPlay = [autoPlayObject boolValue];
        }
    }

    const NSInteger audio_session_result = [self hj_prepareAudioSessionIfNeeded];
    if (audio_session_result < 0) {
        HJFLoge("HJMusicPlayer openURL failed: audio_session_result={}", audio_session_result);
        return audio_session_result;
    }

    HJGraphPlayer::Ptr player_to_release = nullptr;
    BOOL did_create_player = NO;
    NSInteger prepare_result = HJ_OK;
    int result = HJErrNotInited;
    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        if (_handle->player != nullptr && (self.currentURL.length > 0 || !self.prepared)) {
            player_to_release = [self hj_detachPlayerLocked];
        }
    }
    if (player_to_release) {
        player_to_release->done();
    }

    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        prepare_result = [self hj_prepareLocked:&did_create_player];
        if (prepare_result >= 0 && _handle->player) {
            const int playback_state_result = autoPlay ? _handle->player->resume() : _handle->player->pause();
            if (playback_state_result < 0) {
                result = playback_state_result;
            } else {
                HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(HJNSStringToStdString(resolvedURL));
                if (startTime > 0) {
                    (*mediaUrl)["startTimestamp"] = startTime;
                }
                if (audioTrackID >= 0) {
                    (*mediaUrl)["audioTrackID"] = audioTrackID;
                }
                result = _handle->player->openURL(mediaUrl);
                if (result >= 0) {
                    self.currentURL = resolvedURL;
                    self.paused = !autoPlay;
                    [self hj_refreshDurationLockedWithFallback:0];
                    [self hj_refreshAudioTracksLocked];
                    [self hj_refreshMediaInfoLocked];
                }
            }
        }
    }
    if (prepare_result < 0) {
        HJFLoge("HJMusicPlayer openURL failed: prepare_result={}", prepare_result);
        return prepare_result;
    }

    if (did_create_player) {
        [self hj_notifyOnMainThread:^{
            id<HJMusicPlayerDelegate> delegate = self.delegate;
            if ([delegate respondsToSelector:@selector(musicPlayerDidPrepare:)]) {
                [delegate musicPlayerDidPrepare:self];
            }
        }];
    }

    HJFLogi("HJMusicPlayer openURL result: {}", result);
    if (result < 0) {
        HJFLoge("HJMusicPlayer openURL failed: result={}, resolved_url={}",
                result, HJNSStringToStdString(resolvedURL));
        [self hj_notifyErrorCode:result message:@"openURL failed"];
    }
    return result;
}

- (NSInteger)pause {
    HJFLogi("entry");
    int result = HJErrNotInited;
    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        result = _handle->player ? _handle->player->pause() : HJErrNotInited;
        if (result >= 0) {
            self.paused = YES;
        }
    }
    if (result >= 0) {
        [self.audioSessionController handlePlayerDidPause];
        [self hj_notifyOnMainThread:^{
            id<HJMusicPlayerDelegate> delegate = self.delegate;
            if ([delegate respondsToSelector:@selector(musicPlayerDidPause:)]) {
                [delegate musicPlayerDidPause:self];
            }
        }];
    } else {
        HJFLoge("HJMusicPlayer pause failed: result={}", result);
    }
    HJFLogi("end");
    return result;
}

- (NSInteger)resume {
    HJFLogi("entry");
    const NSInteger audio_session_result = [self hj_prepareAudioSessionIfNeeded];
    if (audio_session_result < 0) {
        HJFLoge("HJMusicPlayer resume failed: audio_session_result={}", audio_session_result);
        return audio_session_result;
    }

    int result = HJErrNotInited;
    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        result = _handle->player ? _handle->player->resume() : HJErrNotInited;
        if (result >= 0) {
            self.paused = NO;
        }
    }
    if (result >= 0) {
        [self hj_notifyOnMainThread:^{
            id<HJMusicPlayerDelegate> delegate = self.delegate;
            if ([delegate respondsToSelector:@selector(musicPlayerDidResume:)]) {
                [delegate musicPlayerDidResume:self];
            }
        }];
    } else {
        HJFLoge("HJMusicPlayer resume failed: result={}", result);
    }
    HJFLogi("end");
    return result;
}

- (NSInteger)stop {
    HJFLogi("entry");
    HJGraphPlayer::Ptr player_to_release = nullptr;
    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        player_to_release = [self hj_detachPlayerLocked];
    }
    if (player_to_release) {
        player_to_release->done();
    }
    [self.audioSessionController handlePlayerDidStop];
    [self hj_notifyOnMainThread:^{
        id<HJMusicPlayerDelegate> delegate = self.delegate;
        if ([delegate respondsToSelector:@selector(musicPlayerDidStop:)]) {
            [delegate musicPlayerDidStop:self];
        }
    }];
    HJFLogi("end");
    return HJ_OK;
}

- (NSInteger)seekToMilliseconds:(int64_t)positionMs {
    HJFLogi("HJMusicPlayer seekToMilliseconds: {}", positionMs);
    const int64_t safePosition = std::max<int64_t>(0, positionMs);
    std::lock_guard<std::mutex> guard(_handle->mutex);
    const int result = _handle->player ? _handle->player->seek(safePosition) : HJErrNotInited;
    if (result < 0) {
        HJFLoge("HJMusicPlayer seekToMilliseconds failed: position_ms={}, result={}",
                safePosition, result);
    }
    return result;
}

- (NSInteger)setMute:(BOOL)mute {
    HJFLogi("HJMusicPlayer setMute: {}", mute);
    std::lock_guard<std::mutex> guard(_handle->mutex);
    const int result = _handle->player ? _handle->player->setMute(mute) : HJErrNotInited;
    if (result >= 0) {
        self.muted = mute;
    } else {
        HJFLoge("HJMusicPlayer setMute failed: mute={}, result={}", mute, result);
    }
    return result;
}

- (NSInteger)setVolume:(float)volume {
    HJFLogi("HJMusicPlayer setVolume: %f", volume);
    const float safeVolume = HJClampVolume(volume);
    std::lock_guard<std::mutex> guard(_handle->mutex);
    const int result = _handle->player ? _handle->player->setVolume(safeVolume) : HJErrNotInited;
    if (result >= 0) {
        self.currentVolume = safeVolume;
    } else {
        HJFLoge("HJMusicPlayer setVolume failed: requested_volume={}, safe_volume={}, result={}",
                volume, safeVolume, result);
    }
    return result;
}

- (NSInteger)setRepeats:(NSInteger)repeats {
    HJFLogi("HJMusicPlayer setRepeats: {}", static_cast<int>(repeats));
    if (repeats < 0) {
        HJFLoge("HJMusicPlayer setRepeats failed: repeats={}, result={}",
                static_cast<int>(repeats), HJErrInvalidParams);
        return HJErrInvalidParams;
    }

    int result = HJ_OK;
    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        _repeats = repeats;
        result = _handle->player ? _handle->player->setRepeats(static_cast<int>(repeats)) : HJ_OK;
        if (result == HJErrNotInited) {
            result = HJ_OK;
        }
    }
    if (result < 0) {
        HJFLoge("HJMusicPlayer setRepeats failed: repeats={}, result={}",
                static_cast<int>(repeats), result);
    }
    return result;
}

- (int64_t)getCurrentTimestamp {
    std::lock_guard<std::mutex> guard(_handle->mutex);
    return _handle->player ? _handle->player->getCurrentTimestamp() : 0;
}

- (NSInteger)selectAudioTrack:(NSInteger)trackId {
    HJFLogi("HJMusicPlayer selectAudioTrack: {}", (int)trackId);
    int result = HJErrNotInited;
    BOOL updated_cached_track = NO;
    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        result = _handle->player ? _handle->player->switchAudioTrack(static_cast<int>(trackId)) : HJErrNotInited;
        if (result >= 0) {
            updated_cached_track = [self hj_updateSelectedAudioTrackLocked:trackId];
        }
    }
    if (result >= 0) {
        if (!updated_cached_track) {
            HJFLogw("HJMusicPlayer selectAudioTrack: track_id={} missing from cached audio tracks",
                    static_cast<int>(trackId));
        }
        [self hj_notifyOnMainThread:^{
            id<HJMusicPlayerDelegate> delegate = self.delegate;
            if ([delegate respondsToSelector:@selector(musicPlayer:didUpdateAudioTracks:)]) {
                [delegate musicPlayer:self didUpdateAudioTracks:self.audioTracks];
            }
            if ([delegate respondsToSelector:@selector(musicPlayer:didUpdateMediaInfo:)]) {
                [delegate musicPlayer:self didUpdateMediaInfo:self.mediaInfo];
            }
        }];
    } else {
        HJFLoge("HJMusicPlayer selectAudioTrack failed: track_id={}, result={}",
                static_cast<int>(trackId), result);
    }
    return result;
}

- (void)destroy {
    HJFLogi("HJMusicPlayer destroy");
    HJGraphPlayer::Ptr player_to_release = nullptr;
    {
        std::lock_guard<std::mutex> guard(_handle->mutex);
        player_to_release = [self hj_detachPlayerLocked];
    }
    if (player_to_release) {
        player_to_release->done();
    }
    [self.audioSessionController handlePlayerDidDestroy];
}

- (NSInteger)hj_prepareLocked:(BOOL *)didCreatePlayer {
    HJFLogi("HJMusicPlayer hj_prepareLocked");
    if (didCreatePlayer != NULL) {
        *didCreatePlayer = NO;
    }
    if (self.prepared && _handle->player) {
        return HJ_OK;
    }
    if (![HJPlayerContext sharedInstance].started) {
        HJFLoge("HJMusicPlayer hj_prepareLocked failed: player context not started");
        return HJErrNotInited;
    }

    auto player = HJGraphPlayer::createGraph(HJGraphType_MUSIC, 0);
    if (!player) {
        HJFLoge("HJMusicPlayer hj_prepareLocked failed: createGraph returned nullptr");
        return HJErrNotInited;
    }

    __weak HJMusicPlayer *weakSelf = self;
    auto bus = player->eventBus();
    if (bus) {
        std::weak_ptr<HJGraphPlayer> player_wtr = player;

        const HJRet register_eof_handler_ret = bus->registerHandler(EVENT_GRAPH_EOF_ID, [weakSelf, player_wtr]() {
            HJMusicPlayer *strongSelf = weakSelf;
            if (strongSelf == nil) {
                return;
            }
            [strongSelf hj_notifyOnMainThread:^{
                const auto source_player = player_wtr.lock();
                if (!source_player) {
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(strongSelf->_handle->mutex);
                    if (strongSelf->_handle->player != source_player) {
                        return;
                    }
                }
                id<HJMusicPlayerDelegate> delegate = strongSelf.delegate;
                if ([delegate respondsToSelector:@selector(musicPlayerDidReachEOF:)]) {
                    [delegate musicPlayerDidReachEOF:strongSelf];
                }
            }];
        });
        if (register_eof_handler_ret < 0) {
            HJFLoge("HJMusicPlayer hj_prepareLocked failed: register EVENT_GRAPH_EOF_ID handler, ret={}",
                    register_eof_handler_ret);
        }

        const HJRet register_stream_opened_handler_ret =
            bus->registerHandler(EVENT_GRAPH_STREAM_OPENED_ID, [weakSelf, player_wtr]() {
                HJMusicPlayer *strongSelf = weakSelf;
                if (strongSelf == nil || !strongSelf->_handle) {
                    return;
                }

                [strongSelf hj_dispatchAsyncOnMainThread:^{
                    const auto source_player = player_wtr.lock();
                    if (!source_player) {
                        return;
                    }
                    int64_t latest_duration = source_player->getDuration();
                    NSArray<HJAudioTrackInfo *> *audio_tracks = @[];
                    HJPlayerMediaInfo *media_info = nil;
                    BOOL audio_tracks_changed = NO;
                    BOOL media_info_changed = NO;
                    {
                        std::lock_guard<std::mutex> lock(strongSelf->_handle->mutex);
                        if (strongSelf->_handle->player != source_player) {
                            return;
                        }
                        NSArray<HJAudioTrackInfo *> *previous_audio_tracks =
                            [strongSelf.audioTracks copy];
                        HJPlayerMediaInfo *previous_media_info = strongSelf.mediaInfo;
                        [strongSelf hj_refreshDurationLockedWithFallback:latest_duration];
                        [strongSelf hj_refreshAudioTracksLocked];
                        [strongSelf hj_refreshMediaInfoLocked];
                        latest_duration = strongSelf.duration;
                        audio_tracks = [strongSelf.audioTracks copy];
                        media_info = strongSelf.mediaInfo;
                        audio_tracks_changed =
                            !HJAudioTrackInfoArrayEqual(previous_audio_tracks, audio_tracks);
                        media_info_changed =
                            !HJPlayerMediaInfoEqual(previous_media_info, media_info);
                    }

                    id<HJMusicPlayerDelegate> delegate = strongSelf.delegate;
                    if ([delegate respondsToSelector:@selector(musicPlayer:didUpdateDuration:)]) {
                        [delegate musicPlayer:strongSelf didUpdateDuration:latest_duration];
                    }
                    if (audio_tracks_changed &&
                        [delegate respondsToSelector:@selector(musicPlayer:didUpdateAudioTracks:)]) {
                        [delegate musicPlayer:strongSelf didUpdateAudioTracks:audio_tracks];
                    }
                    if (media_info_changed &&
                        [delegate respondsToSelector:@selector(musicPlayer:didUpdateMediaInfo:)]) {
                        [delegate musicPlayer:strongSelf didUpdateMediaInfo:media_info];
                    }
                }];
            });
        if (register_stream_opened_handler_ret < 0) {
            HJFLoge("HJMusicPlayer hj_prepareLocked failed: register EVENT_GRAPH_STREAM_OPENED_ID handler, ret={}",
                    register_stream_opened_handler_ret);
        }

        const HJRet register_rendered_pcm_handler_ret = bus->registerHandler(EVENT_GRAPH_RENDERED_PCM_ID, [weakSelf, player_wtr](const HJGraphRenderedPCM& pcm) {
            HJMusicPlayer *strongSelf = weakSelf;
            if (strongSelf == nil) {
                return;
            }
            if (!pcm.m_audioInfo || !pcm.m_pcmData || pcm.m_pcmData->empty()) {
                return;
            }

            const auto& audioInfo = pcm.m_audioInfo;
            const auto& pcmData = pcm.m_pcmData;

            NSData *data = [NSData dataWithBytes:pcmData->data() length:pcmData->size()];
            const NSInteger channels = audioInfo->m_channels;
            const NSInteger sampleRate = audioInfo->m_samplesRate;
            const NSInteger sampleFmt = audioInfo->m_sampleFmt;
            const NSInteger trackId = audioInfo->m_trackID;

            [strongSelf hj_dispatchAsyncOnMainThread:^{
                const auto source_player = player_wtr.lock();
                if (!source_player) {
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(strongSelf->_handle->mutex);
                    if (strongSelf->_handle->player != source_player) {
                        return;
                    }
                    if (strongSelf.currentURL.length == 0) {
                        return;
                    }
                }
                id<HJMusicPlayerDelegate> delegate = strongSelf.delegate;
                if ([delegate respondsToSelector:@selector(musicPlayer:didRenderPCMData:channels:sampleRate:sampleFormat:trackId:)]) {
                    [delegate musicPlayer:strongSelf
                         didRenderPCMData:data
                                 channels:channels
                               sampleRate:sampleRate
                             sampleFormat:(HJMusicPlayerSampleFormat)sampleFmt
                                  trackId:trackId];
                }
            }];
        });
        if (register_rendered_pcm_handler_ret < 0) {
            HJFLoge("HJMusicPlayer hj_prepareLocked failed: register EVENT_GRAPH_RENDERED_PCM_ID handler, ret={}",
                    register_rendered_pcm_handler_ret);
        }
    }

    auto param = std::make_shared<HJKeyStorage>();

    const int repeats = static_cast<int>(_repeats);
    const int64_t prerollDurationMs = _options[HJMusicPlayerOptionPrerollDurationMs]
        ? [_options[HJMusicPlayerOptionPrerollDurationMs] longLongValue] : 120;
    const int64_t pcmCallbackIntervalMs = _options[HJMusicPlayerOptionPCMCallbackIntervalMs]
        ? [_options[HJMusicPlayerOptionPCMCallbackIntervalMs] longLongValue] : 20;
    (*param)["repeats"] = repeats;
    (*param)["prerollDurationMs"] = prerollDurationMs;
    (*param)["pcmCallbackIntervalMs"] = pcmCallbackIntervalMs;

    const int sampleRate = _options[HJMusicPlayerOptionSampleRate]
        ? [_options[HJMusicPlayerOptionSampleRate] intValue] : 48000;
    const int channels = _options[HJMusicPlayerOptionChannels]
        ? [_options[HJMusicPlayerOptionChannels] intValue] : 2;
    const int sampleFmt = _options[HJMusicPlayerOptionSampleFormat]
        ? [_options[HJMusicPlayerOptionSampleFormat] intValue]
        : static_cast<int>(HJMusicPlayerSampleFormatS16);

    auto audioInfo = std::make_shared<HJAudioInfo>();
    audioInfo->m_samplesRate = sampleRate;
    audioInfo->setChannels(channels);
    audioInfo->m_sampleFmt = sampleFmt;
    auto bytesForSampleFmt = [](int fmt) -> int {
        switch (fmt) {
            case 0: case 5:               return 1;  // U8, U8P
            case 1: case 6:               return 2;  // S16, S16P
            case 2: case 3: case 7: case 8: return 4; // S32, FLT, S32P, FLTP
            case 4: case 9: case 10: case 11: return 8; // DBL, DBLP, S64, S64P
            default:                      return 2;  // fallback
        }
    };
    audioInfo->m_bytesPerSample = bytesForSampleFmt(sampleFmt);
    (*param)["audioInfo"] = audioInfo;

    const int result = player->init(param);
    if (result < 0) {
        HJFLoge("HJMusicPlayer hj_prepareLocked failed: player init result={}", result);
        player->done();
        return result;
    }

    _handle->player = player;
    self.prepared = YES;
    self.paused = NO;
    self.muted = NO;
    self.currentVolume = 1.0f;
    self.duration = 0;
    self.audioTracks = @[];
    self.mediaInfo = nil;

    if (didCreatePlayer != NULL) {
        *didCreatePlayer = YES;
    }
    return HJ_OK;
}

- (NSInteger)hj_prepareAudioSessionIfNeeded {
    NSError *error = nil;
    if ([self.audioSessionController prepareForPlayback:&error]) {
        return HJ_OK;
    }

    NSString *message = error.localizedDescription ?: @"audio session prepare failed";
    HJFLoge("HJMusicPlayer hj_prepareAudioSessionIfNeeded failed: {}",
            HJNSStringToStdString(message));
    [self hj_notifyErrorCode:HJErrAContext message:message];
    return HJErrAContext;
}

- (HJGraphPlayer::Ptr)hj_detachPlayerLocked {
    HJFLogi("HJMusicPlayer hj_detachPlayerLocked");
    HJGraphPlayer::Ptr player = _handle->player;
    _handle->player.reset();
    [self hj_resetStateLocked];
    return player;
}

- (void)hj_resetStateLocked {
    self.prepared = NO;
    self.paused = NO;
    self.muted = NO;
    self.currentVolume = 1.0f;
    self.duration = 0;
    self.audioTracks = @[];
    self.mediaInfo = nil;
    self.currentURL = nil;
}

- (void)hj_refreshDurationLockedWithFallback:(int64_t)fallbackDuration {
    int64_t latest_duration = fallbackDuration;
    if (_handle->player) {
        latest_duration = _handle->player->getDuration();
        if (latest_duration <= 0) {
            latest_duration = fallbackDuration;
        }
    }
    self.duration = std::max<int64_t>(0, latest_duration);
}

- (void)hj_refreshAudioTracksLocked {
    if (!_handle->player) {
        self.audioTracks = @[];
        return;
    }

    NSMutableArray<HJAudioTrackInfo *> *tracks = [NSMutableArray array];
    HJAudioTrackDisplayInfoVector infos = _handle->player->getAudioTrackDisplayInfos();
    for (const auto &info : infos) {
        if (!info) {
            continue;
        }

        HJAudioTrackInfo *track = [[HJAudioTrackInfo alloc] init];
        track.trackId = info->m_trackID;
        track.displayName = [NSString stringWithUTF8String:info->m_displayName.c_str()] ?: @"";
        track.title = [NSString stringWithUTF8String:info->m_title.c_str()] ?: @"";
        track.language = [NSString stringWithUTF8String:info->m_language.c_str()] ?: @"";
        track.codecName = [NSString stringWithUTF8String:info->m_codecName.c_str()] ?: @"";
        track.channels = info->m_channels;
        track.sampleRate = info->m_sampleRate;
        track.defaultTrack = info->m_isDefault;
        track.selected = info->m_isSelected;
        [tracks addObject:track];
    }
    self.audioTracks = tracks.copy;
}

- (void)hj_refreshMediaInfoLocked {
    HJPlayerMediaInfo *info = [[HJPlayerMediaInfo alloc] init];
    info.url = self.currentURL ?: @"";
    info.duration = self.duration;
    info.trackCount = self.audioTracks.count;

    HJAudioTrackInfo *selectedTrack = nil;
    for (HJAudioTrackInfo *track in self.audioTracks) {
        if (track.isSelected) {
            selectedTrack = track;
            break;
        }
    }
    if (selectedTrack == nil && self.audioTracks.count > 0) {
        selectedTrack = self.audioTracks.firstObject;
    }

    info.audioCodec = selectedTrack.codecName ?: @"";
    info.sampleRate = selectedTrack.sampleRate;
    info.channels = selectedTrack.channels;
    self.mediaInfo = info;
}

- (BOOL)hj_updateSelectedAudioTrackLocked:(NSInteger)trackId {
    if (self.audioTracks.count == 0) {
        return NO;
    }

    BOOL found = NO;
    NSMutableArray<HJAudioTrackInfo *> *tracks =
        [NSMutableArray arrayWithCapacity:self.audioTracks.count];
    for (HJAudioTrackInfo *source_track in self.audioTracks) {
        if (source_track == nil) {
            continue;
        }

        HJAudioTrackInfo *track = [[HJAudioTrackInfo alloc] init];
        track.trackId = source_track.trackId;
        track.displayName = source_track.displayName ?: @"";
        track.title = source_track.title ?: @"";
        track.language = source_track.language ?: @"";
        track.codecName = source_track.codecName ?: @"";
        track.channels = source_track.channels;
        track.sampleRate = source_track.sampleRate;
        track.defaultTrack = source_track.isDefaultTrack;
        track.selected = (track.trackId == trackId);
        found = found || track.isSelected;
        [tracks addObject:track];
    }

    if (!found) {
        return NO;
    }

    self.audioTracks = tracks.copy;
    [self hj_refreshMediaInfoLocked];
    return YES;
}

- (NSString *)hj_resolvedURL:(NSString *)url {
    if (url.length > 0) {
        return url;
    }

    NSArray<NSBundle *> *bundles = @[
        [NSBundle mainBundle],
        [NSBundle bundleForClass:[self class]]
    ];
    for (NSBundle *bundle in bundles) {
        NSString *path = [bundle pathForResource:@"c58733ac51124fe38cdc6540a7b8fa46"
                                          ofType:@"mkv"
                                     inDirectory:@"res"];
        if (path.length > 0) {
            return path;
        }
    }
    return nil;
}

- (void)hj_notifyOnMainThread:(dispatch_block_t)block {
    if (block == nil) {
        return;
    }
#if HJ_MUSIC_PLAYER_CALLBACK_ON_MAIN_THREAD
    dispatch_async(dispatch_get_main_queue(), block);
#else
    block();
#endif
}

- (void)hj_dispatchAsyncOnMainThread:(dispatch_block_t)block {
    if (block == nil) {
        return;
    }
#if HJ_MUSIC_PLAYER_CALLBACK_ON_MAIN_THREAD
    dispatch_async(dispatch_get_main_queue(), block);
#else
    block();
#endif
}

- (void)hj_notifyErrorCode:(NSInteger)errorCode message:(NSString *)message {
    [self hj_notifyOnMainThread:^{
        id<HJMusicPlayerDelegate> delegate = self.delegate;
        if ([delegate respondsToSelector:@selector(musicPlayer:didFailWithErrorCode:message:)]) {
            [delegate musicPlayer:self didFailWithErrorCode:errorCode message:message];
        }
    }];
}

#pragma mark -- HJPlayerAudioSessionControllerOwner

- (BOOL)hj_playerAudioSessionControllerIsPlaying {
    std::lock_guard<std::mutex> guard(_handle->mutex);
    return self.prepared &&
           !self.paused &&
           self.currentURL.length > 0 &&
           _handle->player != nullptr;
}

- (NSInteger)hj_playerAudioSessionControllerPauseForReason:
    (HJPlayerAudioSessionPauseReason)reason {
    (void)reason;
    return [self pause];
}

- (NSInteger)hj_playerAudioSessionControllerResumeForReason:
    (HJPlayerAudioSessionResumeReason)reason {
    (void)reason;
    return [self resume];
}


@end
