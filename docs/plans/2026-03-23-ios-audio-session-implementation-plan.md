# iOS Audio Session Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add an Objective-C `HJAudioSession` wrapper that centralizes `AVAudioSession` configuration, explicit controls, and query APIs for iOS player-side code.

**Architecture:** Keep the public interface in `HJAudioSession.h` as pure Objective-C models, enums, and methods. Implement all Apple-specific mapping and validation in `HJAudioSession.mm`, and drive the change with a standalone ObjC++ test file compiled by `xcrun clang++` so the implementation stays within a small file surface.

**Tech Stack:** Objective-C, Objective-C++, AVFoundation, UIKit-free audio session APIs, standalone `xcrun clang++` test execution.

---

### Task 1: Write the failing audio session test

**Files:**
- Create: `src/entry/player/isys/HJAudioSession_test.mm`
- Test: `src/entry/player/isys/HJAudioSession_test.mm`

**Step 1: Write the failing test**

Cover these behaviors:

- `sharedInstance` exists
- playback configuration can be applied
- `setActive:YES/NO` returns success
- `availableInputs` and `currentRoute` can be queried
- `preferSpeaker` with non-`playAndRecord` category fails

**Step 2: Run test to verify it fails**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -Werror \
  src/entry/player/isys/HJAudioSession_test.mm \
  src/entry/player/isys/HJAudioSession.mm \
  -framework Foundation -framework AVFoundation \
  -o /tmp/hj_audio_session_test
```

Expected:
- Compile fails because `HJAudioSession` API does not exist yet

### Task 2: Implement the public Objective-C API

**Files:**
- Modify: `src/entry/player/isys/HJAudioSession.h`

**Step 1: Add enums and model types**

Implement:

- `HJAudioSessionCategory`
- `HJAudioSessionMode`
- `HJAudioSessionCategoryOptions`
- `HJAudioSessionRouteSharingPolicy`
- `HJAudioSessionSetActiveOptions`
- `HJAudioSessionPortOverride`
- `HJAudioSessionRecordPermission`
- `HJAudioSessionConfiguration`
- `HJAudioSessionPortInfo`
- `HJAudioSessionRouteInfo`

**Step 2: Add the `HJAudioSession` interface**

Implement methods:

- `+ sharedInstance`
- `- applyConfiguration:error:`
- `- setActive:error:`
- `- setActive:options:error:`
- `- setPreferredInputByUID:error:`
- `- overrideOutputAudioPort:error:`
- `- requestRecordPermission:`

Implement query properties:

- `category`
- `mode`
- `categoryOptions`
- `sampleRate`
- `preferredSampleRate`
- `ioBufferDuration`
- `preferredIOBufferDuration`
- `inputLatency`
- `outputLatency`
- `inputNumberOfChannels`
- `outputNumberOfChannels`
- `inputAvailable`
- `otherAudioPlaying`
- `secondaryAudioShouldBeSilencedHint`
- `recordPermission`
- `availableInputs`
- `currentRoute`

### Task 3: Implement the Objective-C++ wrapper

**Files:**
- Modify: `src/entry/player/isys/HJAudioSession.mm`

**Step 1: Add system mapping helpers**

Implement private helpers to map between:

- `HJAudioSessionCategory` and `AVAudioSessionCategory`
- `HJAudioSessionMode` and `AVAudioSessionMode`
- `HJAudioSessionCategoryOptions` and `AVAudioSessionCategoryOptions`
- `HJAudioSessionRouteSharingPolicy` and `AVAudioSessionRouteSharingPolicy`
- `HJAudioSessionSetActiveOptions` and `AVAudioSessionSetActiveOptions`
- `HJAudioSessionPortOverride` and `AVAudioSessionPortOverride`

**Step 2: Add validation and error helpers**

Implement:

- `HJAudioSessionErrorDomain`
- local validation for nil config, unsupported enum mapping, invalid `preferSpeaker`, missing input UID

**Step 3: Add write-path implementation**

Implement:

- singleton initialization
- serial queue protected mutation path
- `applyConfiguration:error:` in the approved order
- `setActive` variants
- `setPreferredInputByUID:error:`
- `overrideOutputAudioPort:error:`
- `requestRecordPermission:`

**Step 4: Add read-path implementation**

Implement:

- current category/mode/categoryOptions mapping
- sample-rate / latency / channel count getters
- record permission mapping
- conversion from `AVAudioSessionPortDescription` to `HJAudioSessionPortInfo`
- conversion from `AVAudioSessionRouteDescription` to `HJAudioSessionRouteInfo`

### Task 4: Run verification and document edge cases

**Files:**
- Test: `src/entry/player/isys/HJAudioSession_test.mm`
- Modify: `src/entry/player/isys/HJAudioSession.h`
- Modify: `src/entry/player/isys/HJAudioSession.mm`

**Step 1: Re-run the standalone test**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -Werror \
  src/entry/player/isys/HJAudioSession_test.mm \
  src/entry/player/isys/HJAudioSession.mm \
  -framework Foundation -framework AVFoundation \
  -o /tmp/hj_audio_session_test

/tmp/hj_audio_session_test
```

Expected:
- compile succeeds
- executable exits with code 0

**Step 2: Run a syntax-only compile of the implementation**

Run:
```bash
xcrun --sdk iphonesimulator clang++ -fsyntax-only -fobjc-arc -Werror \
  src/entry/player/isys/HJAudioSession.mm
```

Expected:
- exit 0

**Step 3: Review edge cases**

Confirm and report:

- nil configuration handling
- invalid `preferSpeaker` combination
- missing preferred input UID
- empty route/available input lists
- unsupported system capabilities on older iOS versions
