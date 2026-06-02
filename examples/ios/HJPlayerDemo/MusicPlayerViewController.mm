#import "MusicPlayerViewController.h"

#import "HJMusicPlayer.h"

namespace {

dispatch_queue_t HJMusicPlayerDestroyQueue() {
    static dispatch_queue_t queue = nil;
    static dispatch_once_t once_token;
    dispatch_once(&once_token, ^{
        queue = dispatch_queue_create("com.hjmedia.playerdemo.music-player.destroy",
                                      DISPATCH_QUEUE_SERIAL);
    });
    return queue;
}

}  // namespace

@interface MusicPlayerViewController () <HJMusicPlayerDelegate>

@property (nonatomic, strong) HJMusicPlayer *player;
@property (nonatomic, strong) UILabel *fileLabel;
@property (nonatomic, strong) UILabel *stateLabel;
@property (nonatomic, strong) UILabel *timeLabel;
@property (nonatomic, strong) UILabel *infoLabel;
@property (nonatomic, strong) UISlider *progressSlider;
@property (nonatomic, strong) UISlider *volumeSlider;
@property (nonatomic, strong) UISwitch *muteSwitch;
@property (nonatomic, strong) UISegmentedControl *repeatControl;
@property (nonatomic, strong) UISegmentedControl *trackControl;
@property (nonatomic, strong) NSTimer *progressTimer;
@property (nonatomic, assign) BOOL isSeeking;
@property (nonatomic, assign) NSInteger selectedRepeats;

- (void)buildUI;
- (UILabel *)makeTitleLabel:(NSString *)title;
- (UILabel *)makeValueLabel;
- (UIButton *)makeButton:(NSString *)title action:(SEL)action;
- (void)startProgressTimer;
- (void)progressTimerFired;
- (void)refreshProgress;
- (void)refreshUI;
- (NSString *)stringFromMilliseconds:(int64_t)ms;
- (NSString *)defaultMediaURL;
- (void)openDefault;
- (void)pausePlayback;
- (void)resumePlayback;
- (void)stopPlayback;
- (void)progressTouchDown;
- (void)progressValueChanged:(UISlider *)slider;
- (void)progressTouchUp:(UISlider *)slider;
- (void)volumeChanged:(UISlider *)slider;
- (void)muteChanged:(UISwitch *)sender;
- (void)repeatChanged:(UISegmentedControl *)sender;
- (void)trackChanged:(UISegmentedControl *)sender;
- (void)destroyPlayerIfNeeded;

@end

@implementation MusicPlayerViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.title = @"Music Player";
    self.view.backgroundColor = [UIColor systemBackgroundColor];
    self.player = [[HJMusicPlayer alloc] initWithDelegate:self options:nil];
    self.selectedRepeats = 0;
    [self.player prepare];

    [self buildUI];
    [self refreshUI];
    [self startProgressTimer];
}

- (void)dealloc {
    [self.progressTimer invalidate];
    [self destroyPlayerIfNeeded];
}

- (void)buildUI {
    UILabel *fileTitle = [self makeTitleLabel:@"Current File"];
    self.fileLabel = [self makeValueLabel];

    UILabel *stateTitle = [self makeTitleLabel:@"State"];
    self.stateLabel = [self makeValueLabel];

    UILabel *timeTitle = [self makeTitleLabel:@"Progress"];
    self.timeLabel = [self makeValueLabel];

    UILabel *infoTitle = [self makeTitleLabel:@"Audio Info"];
    self.infoLabel = [self makeValueLabel];
    self.infoLabel.numberOfLines = 0;

    self.progressSlider = [[UISlider alloc] init];
    [self.progressSlider addTarget:self
                            action:@selector(progressTouchDown)
                  forControlEvents:UIControlEventTouchDown];
    [self.progressSlider addTarget:self
                            action:@selector(progressValueChanged:)
                  forControlEvents:UIControlEventValueChanged];
    [self.progressSlider addTarget:self
                            action:@selector(progressTouchUp:)
                  forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside];

    UIButton *openButton = [self makeButton:@"Open Default" action:@selector(openDefault)];
    UIButton *pauseButton = [self makeButton:@"Pause" action:@selector(pausePlayback)];
    UIButton *resumeButton = [self makeButton:@"Resume" action:@selector(resumePlayback)];
    UIButton *stopButton = [self makeButton:@"Stop" action:@selector(stopPlayback)];

    UILabel *volumeTitle = [self makeTitleLabel:@"Volume"];
    self.volumeSlider = [[UISlider alloc] init];
    self.volumeSlider.minimumValue = 0.0f;
    self.volumeSlider.maximumValue = 1.0f;
    self.volumeSlider.value = 1.0f;
    [self.volumeSlider addTarget:self
                          action:@selector(volumeChanged:)
                forControlEvents:UIControlEventValueChanged];

    UILabel *muteTitle = [self makeTitleLabel:@"Mute"];
    self.muteSwitch = [[UISwitch alloc] init];
    [self.muteSwitch addTarget:self
                        action:@selector(muteChanged:)
              forControlEvents:UIControlEventValueChanged];

    UILabel *repeatTitle = [self makeTitleLabel:@"Repeats"];
    self.repeatControl = [[UISegmentedControl alloc] initWithItems:@[ @"0", @"1", @"2", @"3" ]];
    self.repeatControl.selectedSegmentIndex = self.selectedRepeats;
    [self.repeatControl addTarget:self
                           action:@selector(repeatChanged:)
                 forControlEvents:UIControlEventValueChanged];

    UILabel *trackTitle = [self makeTitleLabel:@"Tracks"];
    self.trackControl = [[UISegmentedControl alloc] initWithItems:@[]];
    [self.trackControl addTarget:self
                          action:@selector(trackChanged:)
                forControlEvents:UIControlEventValueChanged];

    UIStackView *buttonRow =
        [[UIStackView alloc] initWithArrangedSubviews:@[ openButton, pauseButton, resumeButton, stopButton ]];
    buttonRow.axis = UILayoutConstraintAxisHorizontal;
    buttonRow.distribution = UIStackViewDistributionFillEqually;
    buttonRow.spacing = 12.0;

    UIStackView *content = [[UIStackView alloc] initWithArrangedSubviews:@[
        fileTitle, self.fileLabel,
        stateTitle, self.stateLabel,
        timeTitle, self.timeLabel, self.progressSlider,
        infoTitle, self.infoLabel,
        buttonRow,
        volumeTitle, self.volumeSlider,
        muteTitle, self.muteSwitch,
        repeatTitle, self.repeatControl,
        trackTitle, self.trackControl
    ]];
    content.axis = UILayoutConstraintAxisVertical;
    content.spacing = 12.0;
    content.translatesAutoresizingMaskIntoConstraints = NO;

    [self.view addSubview:content];
    [NSLayoutConstraint activateConstraints:@[
        [content.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:20.0],
        [content.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-20.0],
        [content.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:20.0]
    ]];
}

- (UILabel *)makeTitleLabel:(NSString *)title {
    UILabel *label = [[UILabel alloc] init];
    label.font = [UIFont boldSystemFontOfSize:14.0];
    label.text = title;
    return label;
}

- (UILabel *)makeValueLabel {
    UILabel *label = [[UILabel alloc] init];
    label.font = [UIFont systemFontOfSize:14.0];
    label.textColor = [UIColor secondaryLabelColor];
    return label;
}

- (UIButton *)makeButton:(NSString *)title action:(SEL)action {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    [button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
    return button;
}

- (void)startProgressTimer {
    self.progressTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                          target:self
                                                        selector:@selector(progressTimerFired)
                                                        userInfo:nil
                                                         repeats:YES];
}

- (void)progressTimerFired {
    if (self.isSeeking) {
        return;
    }
    [self refreshProgress];
}

- (void)refreshProgress {
    int64_t duration = self.player.duration;
    int64_t position = [self.player getCurrentTimestamp];
    if (self.isSeeking) {
        position = (int64_t)self.progressSlider.value;
    }
    self.timeLabel.text = [NSString stringWithFormat:@"%@ / %@",
                                                     [self stringFromMilliseconds:position],
                                                     [self stringFromMilliseconds:duration]];
    if (duration > 0) {
        self.progressSlider.enabled = YES;
        self.progressSlider.minimumValue = 0.0f;
        self.progressSlider.maximumValue = (float)duration;
        if (!self.isSeeking) {
            self.progressSlider.value = MIN((float)position, (float)duration);
        }
    } else {
        self.progressSlider.enabled = NO;
        if (!self.isSeeking) {
            self.progressSlider.value = 0.0f;
        }
    }
}

- (void)refreshUI {
    NSString *state = @"Idle";
    if (!self.player.isPrepared) {
        state = @"Not Prepared";
    } else if (self.player.isPaused) {
        state = @"Paused";
    } else if (self.player.mediaInfo.url.length > 0) {
        state = @"Playing";
    } else {
        state = @"Ready";
    }
    self.stateLabel.text = state;
    self.volumeSlider.value = self.player.currentVolume;
    self.muteSwitch.on = self.player.isMuted;
    self.repeatControl.selectedSegmentIndex = self.selectedRepeats;

    HJPlayerMediaInfo *info = self.player.mediaInfo;
    NSString *fileName = info.url.lastPathComponent;
    self.fileLabel.text = fileName.length > 0 ? fileName : @"Default bundle resource";
    self.infoLabel.text = [NSString stringWithFormat:@"codec: %@\nsampleRate: %ld\nchannels: %ld\ntracks: %ld",
                                                     info.audioCodec ?: @"",
                                                     (long)info.sampleRate,
                                                     (long)info.channels,
                                                     (long)info.trackCount];

    [self.trackControl removeAllSegments];
    self.trackControl.enabled = (self.player.audioTracks.count > 0);
    [self.player.audioTracks enumerateObjectsUsingBlock:^(HJAudioTrackInfo *obj,
                                                          NSUInteger idx,
                                                          BOOL *stop) {
        NSString *title = obj.displayName.length > 0
            ? obj.displayName
            : [NSString stringWithFormat:@"Track %ld", (long)obj.trackId];
        [self.trackControl insertSegmentWithTitle:title atIndex:idx animated:NO];
        if (obj.isSelected) {
            self.trackControl.selectedSegmentIndex = idx;
        }
        (void)stop;
    }];
    if (self.trackControl.numberOfSegments == 0) {
        self.trackControl.selectedSegmentIndex = UISegmentedControlNoSegment;
    }

    [self refreshProgress];
}

- (NSString *)stringFromMilliseconds:(int64_t)ms {
    if (ms <= 0) {
        return @"00:00";
    }
    int64_t totalSeconds = ms / 1000;
    int64_t minutes = totalSeconds / 60;
    int64_t seconds = totalSeconds % 60;
    return [NSString stringWithFormat:@"%02lld:%02lld", minutes, seconds];
}

- (NSString *)defaultMediaURL {
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

- (void)openDefault {
    NSString *defaultURL = [self defaultMediaURL];
    if (defaultURL.length == 0) {
        self.stateLabel.text = @"Default resource not found";
        return;
    }

    NSInteger result = [self.player openURL:defaultURL
                                    options:@{
                                        HJMusicPlayerOpenOptionStartTime : @0,
                                        HJMusicPlayerOpenOptionAudioTrackID : @1,
                                    }];
    if (result < 0) {
        self.stateLabel.text = [NSString stringWithFormat:@"Open failed: %ld", (long)result];
        return;
    }
    [self refreshUI];
}

- (void)pausePlayback {
    [self.player pause];
    [self refreshUI];
}

- (void)resumePlayback {
    [self.player resume];
    [self refreshUI];
}

- (void)stopPlayback {
    [self.player stop];
    [self refreshUI];
}

- (void)progressTouchDown {
    self.isSeeking = YES;
}

- (void)progressValueChanged:(UISlider *)slider {
    (void)slider;
    if (!self.isSeeking) {
        return;
    }
    [self refreshProgress];
}

- (void)progressTouchUp:(UISlider *)slider {
    const float seekValue = slider.value;
    self.isSeeking = NO;
    [self.player seekToMilliseconds:(int64_t)seekValue];
    [self refreshProgress];
}

- (void)volumeChanged:(UISlider *)slider {
    [self.player setVolume:slider.value];
    [self refreshUI];
}

- (void)muteChanged:(UISwitch *)sender {
    [self.player setMute:sender.isOn];
    [self refreshUI];
}

- (void)repeatChanged:(UISegmentedControl *)sender {
    NSInteger repeats = sender.selectedSegmentIndex;
    if (repeats < 0) {
        sender.selectedSegmentIndex = self.selectedRepeats;
        return;
    }

    NSInteger result = [self.player setRepeats:repeats];
    if (result >= 0) {
        self.selectedRepeats = repeats;
    } else {
        sender.selectedSegmentIndex = self.selectedRepeats;
    }
    [self refreshUI];
}

- (void)trackChanged:(UISegmentedControl *)sender {
    NSInteger index = sender.selectedSegmentIndex;
    if (index < 0 || index >= (NSInteger)self.player.audioTracks.count) {
        return;
    }
    HJAudioTrackInfo *track = self.player.audioTracks[(NSUInteger)index];
    [self.player selectAudioTrack:track.trackId];
}

- (void)destroyPlayerIfNeeded {
    HJMusicPlayer *player = self.player;
    if (player == nil) {
        return;
    }

    player.delegate = nil;
    self.player = nil;
    dispatch_async(HJMusicPlayerDestroyQueue(), ^{
        [player destroy];
    });
}

- (void)musicPlayerDidPrepare:(HJMusicPlayer *)player {
    (void)player;
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshUI]; });
}

- (void)musicPlayerDidPause:(HJMusicPlayer *)player {
    (void)player;
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshUI]; });
}

- (void)musicPlayerDidResume:(HJMusicPlayer *)player {
    (void)player;
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshUI]; });
}

- (void)musicPlayerDidStop:(HJMusicPlayer *)player {
    (void)player;
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshUI]; });
}

- (void)musicPlayerDidReachEOF:(HJMusicPlayer *)player {
    (void)player;
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshUI]; });
}

- (void)musicPlayer:(HJMusicPlayer *)player
 didFailWithErrorCode:(NSInteger)errorCode
             message:(NSString *)message {
    (void)player;
    dispatch_async(dispatch_get_main_queue(), ^{
        self.stateLabel.text = [NSString stringWithFormat:@"Error %ld: %@",
                                                          (long)errorCode,
                                                          message];
    });
}

- (void)musicPlayer:(HJMusicPlayer *)player didUpdateDuration:(int64_t)duration {
    (void)player;
    (void)duration;
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshProgress]; });
}

- (void)musicPlayer:(HJMusicPlayer *)player
didUpdateAudioTracks:(NSArray<HJAudioTrackInfo *> *)audioTracks {
    (void)player;
    (void)audioTracks;
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshUI]; });
}

- (void)musicPlayer:(HJMusicPlayer *)player
 didUpdateMediaInfo:(HJPlayerMediaInfo *)mediaInfo {
    (void)player;
    (void)mediaInfo;
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshUI]; });
}

- (void)musicPlayer:(HJMusicPlayer *)player
    didRenderPCMData:(NSData *)pcmData
            channels:(NSInteger)channels
          sampleRate:(NSInteger)sampleRate
        sampleFormat:(HJMusicPlayerSampleFormat)sampleFormat
             trackId:(NSInteger)trackId {
    (void)player;
    NSLog(@"PCM callback: %lu bytes, ch=%ld, sr=%ld, fmt=%ld, track=%ld",
          (unsigned long)pcmData.length,
          (long)channels,
          (long)sampleRate,
          (long)sampleFormat,
          (long)trackId);
}

@end
