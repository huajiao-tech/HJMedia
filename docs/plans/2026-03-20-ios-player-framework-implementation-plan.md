# iOS Player Framework Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build `HJPlayer.framework` for iOS from `src/entry/player`, export pure Objective-C APIs `HJMusicPlayer` and `HJPlayerContext`, and add a unified CMake-produced `HJPlayerDemo` app that plays the bundled default MKV resource.

**Architecture:** Keep the existing `HJPlayer` C++ target as the single framework target and add an iOS wrapper layer under `src/entry/player/isys`. The wrapper translates Objective-C calls into `HJGraphMusicPlayer` and `HJEntryContext`, while `examples/ios/HJPlayerDemo` links the framework and exercises playback, seek, volume, mute, track selection, and basic media info display.

**Tech Stack:** CMake, Xcode generator, Objective-C, Objective-C++, C++17-compatible existing core, UIKit, existing `HJGraphMusicPlayer` / `HJEntryContext`.

---

### Task 1: Wire iOS Build Targets

**Files:**
- Modify: `src/entry/player/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Create: `examples/ios/HJPlayerDemo/CMakeLists.txt`

**Step 1: Verify the new iOS app target is currently missing**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayerDemo --config Debug
```

Expected:
- `./build_ios.sh` succeeds and regenerates the Xcode project
- `cmake --build ... --target HJPlayerDemo` fails because the target does not exist yet

**Step 2: Update `src/entry/player/CMakeLists.txt`**

Implement these changes:

- Under `if (IOS)` or `if (APPLE AND IOS)`, include `src/entry/player/isys`
- Add `isys/*.mm` to `SOURCE_FILES`
- Add `isys` include directory
- Set public headers to only:

```cmake
set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/isys/HJMusicPlayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/isys/HJPlayerContext.h)
```

**Step 3: Update root `CMakeLists.txt`**

In the `if (IOS)` block, add:

```cmake
add_subdirectory(examples/ios/HJPlayerDemo)
```

Keep the existing `examples/ios/HJMediaUI` target.

**Step 4: Create `examples/ios/HJPlayerDemo/CMakeLists.txt`**

Implement an iOS app target similar to `examples/ios/HJMediaUI/CMakeLists.txt`, but trimmed for player demo only:

- Target name: `HJPlayerDemo`
- Input sources:
  - `main.m`
  - `AppDelegate.mm`
  - `ViewController.mm`
  - `SceneDelegate.mm`
- Resource source:
  - `res/c58733ac51124fe38cdc6540a7b8fa46.mkv`
- Link:
  - `HJPlayer`
  - UIKit-family frameworks needed by the demo
- Post-build embed:
  - copy `HJPlayer.framework` into app bundle `Frameworks`

Skeleton:

```cmake
set(PROJ_NAME HJPlayerDemo)

file(GLOB UI_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB UI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mm")

set(INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in ${INFO_PLIST} @ONLY)

set(DEMO_RESOURCE ${CMAKE_CURRENT_SOURCE_DIR}/res/c58733ac51124fe38cdc6540a7b8fa46.mkv)
set_source_files_properties(${DEMO_RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/res)

add_executable(${PROJ_NAME} MACOSX_BUNDLE
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${DEMO_RESOURCE})

target_link_libraries(${PROJ_NAME}
    PRIVATE
        HJPlayer
        ${UIKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK})
```

**Step 5: Re-run configure/build to verify target generation**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayer --config Debug
cmake --build build_ios --target HJPlayerDemo --config Debug
```

Expected:
- Both targets are now recognized by CMake/Xcode
- Build may still fail due to missing implementation files and imports, which is acceptable at this stage

**Step 6: Commit**

```bash
git add CMakeLists.txt src/entry/player/CMakeLists.txt examples/ios/HJPlayerDemo/CMakeLists.txt
git commit -m "build: add ios player framework and demo targets"
```

### Task 2: Add `HJPlayerContext` Objective-C API

**Files:**
- Create: `src/entry/player/isys/HJPlayerContext.h`
- Create: `src/entry/player/isys/HJPlayerContext.mm`

**Step 1: Verify the public header cannot yet be imported**

Run:
```bash
rg -n "HJPlayerContext" src/entry/player/isys examples/ios/HJPlayerDemo
```

Expected:
- No iOS Objective-C `HJPlayerContext` wrapper exists yet

**Step 2: Create `HJPlayerContext.h`**

Implement a pure Objective-C public header:

```objective-c
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HJPlayerContextConfig : NSObject
@property (nonatomic, assign) BOOL logEnabled;
@property (nonatomic, copy) NSString *logDirectory;
@property (nonatomic, assign) NSInteger logLevel;
@property (nonatomic, assign) NSInteger logMode;
@property (nonatomic, assign) NSInteger logMaxFileSize;
@property (nonatomic, assign) NSInteger logMaxFileCount;
@property (nonatomic, copy) NSString *mediasDirectory;
@property (nonatomic, assign) NSInteger mediasCacheMax;
@property (nonatomic, assign) NSInteger downloadRetryMax;
@end

@interface HJPlayerContext : NSObject
+ (instancetype)sharedInstance;
- (NSInteger)startWithConfig:(nullable HJPlayerContextConfig *)config;
- (void)stop;
@property (nonatomic, assign, readonly, getter=isStarted) BOOL started;
@end

NS_ASSUME_NONNULL_END
```

**Step 3: Create `HJPlayerContext.mm`**

Implement:

- `sharedInstance`
- default config fallback when `config == nil`
- Objective-C -> `HJEntryContextPlayerInfo` mapping
- single-start behavior with cached result

Key logic:

```objective-c++
HJEntryContextPlayerInfo info;
info.logIsValid = config.logEnabled;
info.logDir = std::string(config.logDirectory.UTF8String ?: "");
info.logLevel = (int)config.logLevel;
info.logMode = (int)config.logMode;
info.logMaxFileSize = (int)config.logMaxFileSize;
info.logMaxFileNum = (int)config.logMaxFileCount;
info.medias_dir = std::string(config.mediasDirectory.UTF8String ?: "");
info.medias_cache_max = (int)config.mediasCacheMax;
info.download_retry_max = (int)config.downloadRetryMax;
int ret = HJEntryContext::init(HJEntryType_Player, info);
```

**Step 4: Rebuild `HJPlayer` to verify header/export compiles**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayer --config Debug
```

Expected:
- `HJPlayerContext` compiles
- build may still fail on missing `HJMusicPlayer` references, which is acceptable

**Step 5: Commit**

```bash
git add src/entry/player/isys/HJPlayerContext.h src/entry/player/isys/HJPlayerContext.mm
git commit -m "feat: add ios player context wrapper"
```

### Task 3: Add `HJMusicPlayer` Public API and Models

**Files:**
- Create: `src/entry/player/isys/HJMusicPlayer.h`

**Step 1: Verify the player public header does not exist**

Run:
```bash
test -f src/entry/player/isys/HJMusicPlayer.h && echo EXISTS || echo MISSING
```

Expected:
- `MISSING`

**Step 2: Create `HJMusicPlayer.h`**

Implement a pure Objective-C API with delegate and model types.

Required declarations:

- `HJAudioTrackInfo`
- `HJPlayerMediaInfo`
- `@protocol HJMusicPlayerDelegate`
- `@interface HJMusicPlayer`

Suggested header skeleton:

```objective-c
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class HJMusicPlayer;

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
@end

@interface HJMusicPlayer : NSObject
- (instancetype)initWithDelegate:(nullable id<HJMusicPlayerDelegate>)delegate;
- (NSInteger)prepare;
- (NSInteger)openURL:(nullable NSString *)url;
- (NSInteger)pause;
- (NSInteger)resume;
- (NSInteger)stop;
- (NSInteger)seekToMilliseconds:(int64_t)positionMs;
- (NSInteger)setMute:(BOOL)mute;
- (NSInteger)setVolume:(float)volume;
- (int64_t)getCurrentTimestamp;
- (NSInteger)selectAudioTrack:(NSInteger)trackId;
- (void)destroy;
@property (nonatomic, weak, nullable) id<HJMusicPlayerDelegate> delegate;
@property (nonatomic, assign, readonly, getter=isPrepared) BOOL prepared;
@property (nonatomic, assign, readonly, getter=isPaused) BOOL paused;
@property (nonatomic, assign, readonly, getter=isMuted) BOOL muted;
@property (nonatomic, assign, readonly) float volume;
@property (nonatomic, assign, readonly) int64_t duration;
@property (nonatomic, copy, readonly) NSArray<HJAudioTrackInfo *> *audioTracks;
@property (nonatomic, strong, readonly, nullable) HJPlayerMediaInfo *mediaInfo;
@end

NS_ASSUME_NONNULL_END
```

**Step 3: Build to confirm the public API is syntactically valid**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayer --config Debug
```

Expected:
- Header parses
- Build still fails because `HJMusicPlayer.mm` is not implemented yet

**Step 4: Commit**

```bash
git add src/entry/player/isys/HJMusicPlayer.h
git commit -m "feat: add ios music player public api"
```

### Task 4: Implement `HJMusicPlayer.mm`

**Files:**
- Create: `src/entry/player/isys/HJMusicPlayer.mm`

**Step 1: Verify `HJMusicPlayer` target still lacks implementation**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayer --config Debug
```

Expected:
- Build fails on missing `HJMusicPlayer` implementation or unresolved symbols

**Step 2: Implement internal handle and lifecycle**

Add a private C++/ObjC++ bridge object inside `HJMusicPlayer.mm` containing:

- `std::mutex`
- `HJGraphPlayer::Ptr player`
- cached `duration`
- cached `audioTracks`
- cached `mediaInfo`
- `BOOL prepared`
- `BOOL paused`
- `BOOL muted`
- `float volume`
- current URL

**Step 3: Implement `prepare`**

Implement:

- check `HJPlayerContext.sharedInstance.started`
- create graph with `HJGraphPlayer::createGraph(HJGraphType_MUSIC, 0)`
- build `HJKeyStorage`
- set:

```objective-c++
auto param = std::make_shared<HJKeyStorage>();
(*param)["repeats"] = 0;
(*param)["prerollDurationMs"] = static_cast<int64_t>(120);
(*param)["pcmCallbackIntervalMs"] = static_cast<int64_t>(20);

auto audioInfo = std::make_shared<HJAudioInfo>();
audioInfo->m_samplesRate = 48000;
audioInfo->setChannels(2);
audioInfo->m_sampleFmt = 1;
audioInfo->m_bytesPerSample = 2;
(*param)["audioInfo"] = audioInfo;
```

- call `player->init(param)`

**Step 4: Register event bus handlers**

Map:

- `EVENT_GRAPH_DURATION_ID` -> update cache, main-thread delegate callback
- `EVENT_GRAPH_EOF_ID` -> main-thread delegate callback

Use `dispatch_async(dispatch_get_main_queue(), ^{ ... })` for delegate calls.

**Step 5: Implement core player methods**

Implement:

- `openURL:` with empty-string fallback to default bundled MKV
- `pause`
- `resume`
- `stop`
- `seekToMilliseconds:` with negative clamp to zero
- `setMute:`
- `setVolume:` with clamp to `0.0f .. 1.0f`
- `getCurrentTimestamp`
- `selectAudioTrack:`
- `destroy`

For default resource lookup:

```objective-c
NSString *path = [[NSBundle bundleForClass:[HJMusicPlayer class]]
    pathForResource:@"c58733ac51124fe38cdc6540a7b8fa46"
             ofType:@"mkv"
        inDirectory:@"res"];
```

**Step 6: Export audio tracks and media info**

Populate Objective-C models from:

- `player->getAudioTrackDisplayInfos()`
- duration cache
- selected/current URL

Refresh model cache after `openURL:` and after successful `selectAudioTrack:`.

**Step 7: Rebuild framework to verify end-to-end wrapper compiles**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayer --config Debug
```

Expected:
- `HJPlayer.framework` builds successfully or only fails on the demo target if the app files are not yet present

**Step 8: Commit**

```bash
git add src/entry/player/isys/HJMusicPlayer.mm
git commit -m "feat: implement ios music player wrapper"
```

### Task 5: Build `HJPlayerDemo`

**Files:**
- Create: `examples/ios/HJPlayerDemo/AppDelegate.h`
- Create: `examples/ios/HJPlayerDemo/AppDelegate.mm`
- Create: `examples/ios/HJPlayerDemo/ViewController.h`
- Create: `examples/ios/HJPlayerDemo/ViewController.mm`
- Create: `examples/ios/HJPlayerDemo/SceneDelegate.h`
- Create: `examples/ios/HJPlayerDemo/SceneDelegate.mm`
- Create: `examples/ios/HJPlayerDemo/main.m`
- Create: `examples/ios/HJPlayerDemo/Info.plist.in`

**Step 1: Verify the app target still cannot build**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayerDemo --config Debug
```

Expected:
- Build fails due to missing app source files

**Step 2: Add app bootstrap files**

Implement minimal UIKit app bootstrap:

- `main.m`
- `AppDelegate`
- `SceneDelegate`

In `AppDelegate.mm`, initialize player context:

```objective-c
HJPlayerContextConfig *config = [HJPlayerContextConfig new];
config.logEnabled = YES;
config.logDirectory = NSTemporaryDirectory();
config.logMode = HJLogLMode_CONSOLE;
config.logMaxFileSize = 5 * 1024 * 1024;
config.logMaxFileCount = 2;
[[HJPlayerContext sharedInstance] startWithConfig:config];
```

**Step 3: Implement `ViewController`**

Build a single-screen UI containing:

- current file label
- state label
- duration label
- current time label
- codec/sample rate/channels label
- progress slider
- buttons:
  - `Open Default`
  - `Pause`
  - `Resume`
  - `Stop`
- volume slider
- mute switch
- optional audio track segmented control

**Step 4: Implement player interaction**

In `ViewController.mm`:

- own one `HJMusicPlayer`
- set self as delegate
- on `Open Default`, call `openURL:nil`
- use `NSTimer` at `0.5s` interval to query `getCurrentTimestamp`
- update progress slider and current time label
- on slider drag end, call `seekToMilliseconds:`
- on volume slider change, call `setVolume:`
- on mute switch change, call `setMute:`

**Step 5: Rebuild the full demo app**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayerDemo --config Debug
```

Expected:
- `HJPlayerDemo` builds successfully

**Step 6: Commit**

```bash
git add examples/ios/HJPlayerDemo
git commit -m "feat: add ios player demo app"
```

### Task 6: Verify Packaging and Runtime Assumptions

**Files:**
- Modify as needed: `examples/ios/HJPlayerDemo/CMakeLists.txt`
- Modify as needed: `src/entry/player/CMakeLists.txt`

**Step 1: Verify framework embedding and resource placement**

Run:
```bash
find build_ios -path "*HJPlayerDemo.app*" | sed -n '1,40p'
find build_ios -path "*HJPlayer.framework*" | sed -n '1,40p'
```

Expected:
- `HJPlayer.framework` exists under the build output
- `HJPlayerDemo.app/Frameworks/HJPlayer.framework` exists
- the bundled MKV exists inside app resources

**Step 2: Smoke-check resource bundle lookup**

Run:
```bash
find build_ios -path "*c58733ac51124fe38cdc6540a7b8fa46.mkv" | sed -n '1,20p'
```

Expected:
- The default MKV appears inside the built app bundle

**Step 3: Fix packaging gaps if any**

Adjust:

- `MACOSX_PACKAGE_LOCATION`
- post-build framework copy
- framework search paths / runpath search paths

until the app bundle layout is correct.

**Step 4: Final build verification**

Run:
```bash
./build_ios.sh
cmake --build build_ios --target HJPlayer --config Debug
cmake --build build_ios --target HJPlayerDemo --config Debug
```

Expected:
- Both targets build successfully

**Step 5: Manual runtime checklist**

Open the generated Xcode project and validate:

1. Launch `HJPlayerDemo`
2. Confirm `HJPlayerContext` initializes at app startup
3. Tap `Open Default`
4. Confirm the bundled MKV starts playback
5. Confirm pause/resume/stop work
6. Confirm progress slider seek works
7. Confirm volume `0.0 .. 1.0` works and mute toggles correctly
8. Confirm audio info renders and updates

**Step 6: Commit**

```bash
git add src/entry/player/CMakeLists.txt examples/ios/HJPlayerDemo/CMakeLists.txt
git commit -m "chore: finalize ios player packaging verification"
```

### Task 7: Document Residual Risks and Follow-Ups

**Files:**
- Modify: `docs/plans/2026-03-20-ios-player-framework-design.md`

**Step 1: Record implementation deviations, if any**

Append a short “Implementation Notes” section describing:

- any API adjustments
- any build-system caveats
- any runtime limitations discovered during packaging/runtime validation

**Step 2: Record post-MVP follow-ups**

List optional follow-ups such as:

- richer `HJPlayerMediaInfo`
- `XCTest` or Objective-C smoke test target
- better stop/close semantics in `HJGraphMusicPlayer`
- Swift overlay or module map cleanup

**Step 3: Commit**

```bash
git add docs/plans/2026-03-20-ios-player-framework-design.md
git commit -m "docs: record ios player implementation notes"
```
