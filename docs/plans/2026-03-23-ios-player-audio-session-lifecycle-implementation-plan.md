# iOS Player Audio Session Lifecycle Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add reusable iOS-side audio-session lifecycle handling for `HJMusicPlayer`, including interruption, route change, and optional internal `AudioSession` management.

**Architecture:** Keep `HJLifeCycle` as a notification forwarder only, add a reusable `HJPlayerAudioSessionController` to encapsulate policy and session coordination, and wire `HJMusicPlayer` to the controller through a small owner protocol. The new player option `HJMusicPlayerOptionManageAudioSession` defaults to `NO`, so player state protection remains internal while `AVAudioSession` configuration ownership can stay external.

**Tech Stack:** Objective-C, Objective-C++, AVFAudio, Foundation notifications, standalone `xcrun clang++` tests, existing `HJAudioSession` and `HJLifeCycle`.

---

### Task 1: Extend `HJLifeCycle` to forward route-change events

**Files:**
- Modify: `src/entry/player/isys/HJLifeCycle.h`
- Modify: `src/entry/player/isys/HJLifeCycle.mm`

**Step 1: Write the failing test**

Create a temporary compile check in `/tmp/hj_lifecycle_route_change_test.mm` that imports `HJLifeCycle.h` and implements:

```objective-c
- (void)audioSessionRouteChange:(NSNotification *)notification;
```

The test should call:

```objective-c
[HJLifeCycle registerLifeCycleListener:listener];
[HJLifeCycle unregisterLifeCycleListener:listener];
```

**Step 2: Run test to verify it fails**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -Werror \
  /tmp/hj_lifecycle_route_change_test.mm \
  src/entry/player/isys/HJLifeCycle.mm \
  -framework Foundation -framework UIKit -framework AVFoundation \
  -o /tmp/hj_lifecycle_route_change_test
```

Expected:
- compile fails because `audioSessionRouteChange:` is not declared/registered yet

**Step 3: Write minimal implementation**

Update `HJLifeCycle.h`:

- add `- (void)audioSessionRouteChange:(NSNotification *)notification;`

Update `HJLifeCycle.mm`:

- register `AVAudioSessionRouteChangeNotification`
- unregister `AVAudioSessionRouteChangeNotification`

**Step 4: Run test to verify it passes**

Run the same compile command again.

Expected:
- compile succeeds

**Step 5: Commit**

```bash
git add src/entry/player/isys/HJLifeCycle.h src/entry/player/isys/HJLifeCycle.mm
git commit -m "feat: forward audio session route change lifecycle events"
```

### Task 2: Add `HJPlayerAudioSessionController` with controller-level tests

**Files:**
- Create: `src/entry/player/isys/HJPlayerAudioSessionController.h`
- Create: `src/entry/player/isys/HJPlayerAudioSessionController.mm`
- Create: `src/entry/player/isys/HJPlayerAudioSessionController_test.mm`

**Step 1: Write the failing test**

In `src/entry/player/isys/HJPlayerAudioSessionController_test.mm`, create:

- a fake owner implementing the controller owner protocol
- counters for `pause` and `resume`
- tests for:
  - `manageAudioSession = NO` does not try to configure/activate session
  - interruption begin pauses only if owner is playing
  - interruption end resumes only when `shouldResume` is present and interruption began while playing
  - route change with `OldDeviceUnavailable` pauses

Minimal pattern:

```objective-c
controller = [[HJPlayerAudioSessionController alloc] initWithOwner:owner
                                                 manageAudioSession:NO];
[controller startObserving];
[controller audioSessionInterrupt:beginNotification];
NSCAssert(owner.pauseCount == 1, @"should pause on interruption begin");
```

**Step 2: Run test to verify it fails**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -Werror \
  src/entry/player/isys/HJPlayerAudioSessionController_test.mm \
  src/entry/player/isys/HJPlayerAudioSessionController.mm \
  src/entry/player/isys/HJLifeCycle.mm \
  src/entry/player/isys/HJAudioSession.mm \
  -framework Foundation -framework UIKit -framework AVFoundation \
  -o /tmp/hj_player_audio_session_controller_test
```

Expected:
- compile fails because the controller/protocol does not exist yet

**Step 3: Write minimal implementation**

Create `HJPlayerAudioSessionController.h`:

- define pause/resume reason enums
- define owner protocol
- define controller interface

Create `HJPlayerAudioSessionController.mm`:

- implement init with owner + `manageAudioSession`
- implement `startObserving` / `stopObserving`
- implement `prepareForPlayback`, `handlePlayerDidPause`, `handlePlayerDidStop`, `handlePlayerDidDestroy`
- implement `audioSessionInterrupt:` and `audioSessionRouteChange:`
- manage internal state:
  - `m_manage_audio_session`
  - `m_was_playing_before_interruption`
  - `m_paused_by_interruption`
  - `m_paused_by_route_change`

Keep first pass simple:

- `prepareForPlayback` configures `HJAudioSession` only when `manageAudioSession == YES`
- `pause/stop/destroy` deactivate session only when `manageAudioSession == YES`

**Step 4: Run test to verify it passes**

Run the compile command again, then attempt execution:

```bash
/tmp/hj_player_audio_session_controller_test
```

Expected:
- compile succeeds
- if simulator runtime is unavailable in environment, record that limitation explicitly

**Step 5: Commit**

```bash
git add src/entry/player/isys/HJPlayerAudioSessionController.h \
        src/entry/player/isys/HJPlayerAudioSessionController.mm \
        src/entry/player/isys/HJPlayerAudioSessionController_test.mm
git commit -m "feat: add reusable player audio session controller"
```

### Task 3: Integrate controller into `HJMusicPlayer`

**Files:**
- Modify: `src/entry/player/isys/HJMusicPlayer.h`
- Modify: `src/entry/player/isys/HJMusicPlayer.mm`
- Create: `src/entry/player/isys/HJMusicPlayer_lifecycle_test.mm`

**Step 1: Write the failing test**

In `src/entry/player/isys/HJMusicPlayer_lifecycle_test.mm`, cover:

- `HJMusicPlayerOptionManageAudioSession` default is `NO`
- player owns a controller and starts observing
- interruption-driven pause/resume delegates through player-facing methods
- `destroy` stops observing and clears state

Minimal pattern:

```objective-c
HJMusicPlayer *player = [[HJMusicPlayer alloc] initWithDelegate:nil options:nil];
NSCAssert(/* manageAudioSession default resolves to NO */, @"default should be NO");
```

If direct black-box validation is too tight, expose minimal internal test hooks in a class extension inside the `.mm` test only.

**Step 2: Run test to verify it fails**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -Werror \
  src/entry/player/isys/HJMusicPlayer_lifecycle_test.mm \
  src/entry/player/isys/HJMusicPlayer.mm \
  src/entry/player/isys/HJPlayerAudioSessionController.mm \
  src/entry/player/isys/HJLifeCycle.mm \
  src/entry/player/isys/HJAudioSession.mm \
  -framework Foundation -framework UIKit -framework AVFoundation \
  -o /tmp/hj_music_player_lifecycle_test
```

Expected:
- compile fails because player option/controller integration is missing

**Step 3: Write minimal implementation**

Update `HJMusicPlayer.h`:

- add:

```objective-c
extern NSString * const HJMusicPlayerOptionManageAudioSession;
```

Update `HJMusicPlayer.mm`:

- import `HJPlayerAudioSessionController.h`
- add controller ivar/property
- parse `HJMusicPlayerOptionManageAudioSession`, default to `NO`
- initialize controller in `init`
- call `startObserving`
- before `prepare/openURL/resume`, call controller `prepareForPlayback`
- after `pause`, call `handlePlayerDidPause`
- after `stop`, call `handlePlayerDidStop`
- in `destroy` and `dealloc`, call `handlePlayerDidDestroy` / `stopObserving`
- implement controller owner protocol methods using existing player `paused/prepared/currentURL` state

Keep behavior minimal and aligned to design:

- background does not auto-pause
- interruption begin pauses when currently playing
- interruption end resumes only when `shouldResume` and interruption paused an active playback
- `OldDeviceUnavailable` pauses

**Step 4: Run test to verify it passes**

Run the same compile command again.

Expected:
- compile succeeds

**Step 5: Commit**

```bash
git add src/entry/player/isys/HJMusicPlayer.h \
        src/entry/player/isys/HJMusicPlayer.mm \
        src/entry/player/isys/HJMusicPlayer_lifecycle_test.mm
git commit -m "feat: integrate player audio session lifecycle controller"
```

### Task 4: Verify end-to-end compile health and report edge cases

**Files:**
- Test: `src/entry/player/isys/HJPlayerAudioSessionController_test.mm`
- Test: `src/entry/player/isys/HJMusicPlayer_lifecycle_test.mm`
- Modify: `src/entry/player/isys/HJMusicPlayer.mm`

**Step 1: Compile the controller test target**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -Werror \
  src/entry/player/isys/HJPlayerAudioSessionController_test.mm \
  src/entry/player/isys/HJPlayerAudioSessionController.mm \
  src/entry/player/isys/HJLifeCycle.mm \
  src/entry/player/isys/HJAudioSession.mm \
  -framework Foundation -framework UIKit -framework AVFoundation \
  -o /tmp/hj_player_audio_session_controller_test
```

Expected:
- exit 0

**Step 2: Compile the player lifecycle test target**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -Werror \
  src/entry/player/isys/HJMusicPlayer_lifecycle_test.mm \
  src/entry/player/isys/HJMusicPlayer.mm \
  src/entry/player/isys/HJPlayerAudioSessionController.mm \
  src/entry/player/isys/HJLifeCycle.mm \
  src/entry/player/isys/HJAudioSession.mm \
  -framework Foundation -framework UIKit -framework AVFoundation \
  -o /tmp/hj_music_player_lifecycle_test
```

Expected:
- exit 0

**Step 3: Run syntax-only compile on the touched implementation files**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fsyntax-only -fobjc-arc -Werror src/entry/player/isys/HJLifeCycle.mm
xcrun --sdk iphonesimulator clang++ -fsyntax-only -fobjc-arc -Werror src/entry/player/isys/HJPlayerAudioSessionController.mm
xcrun --sdk iphonesimulator clang++ -fsyntax-only -fobjc-arc -Werror src/entry/player/isys/HJMusicPlayer.mm
```

Expected:
- all exit 0

**Step 4: Review edge cases**

Confirm and report:

- option default path leaves `AudioSession` configuration external
- interruption end does not resume if playback was already paused before interruption
- route change only pauses on `OldDeviceUnavailable`
- `destroy` unregisters lifecycle observation
- multiple `prepare/openURL/resume` calls do not duplicate registration
