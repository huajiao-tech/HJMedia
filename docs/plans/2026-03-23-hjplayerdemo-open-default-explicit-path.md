# HJPlayerDemo Open Default Explicit Path Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make `HJPlayerDemo` pass an explicit default media path to `openURL:` instead of `nil`.

**Architecture:** Keep the change in the demo layer. Add a small default-resource resolver in `ViewController.mm`, then have `openDefault` use the resolved bundle path and surface an error if the resource is missing. Validate the behavior with a standalone Objective-C++ regression test that stubs `HJMusicPlayer` and captures the `openURL:` argument.

**Tech Stack:** Objective-C++, UIKit, Foundation, manual `xcrun clang++` regression test

---

### Task 1: Regression Test

**Files:**
- Create: `examples/ios/HJPlayerDemo/ViewController_test.mm`
- Test: `examples/ios/HJPlayerDemo/ViewController_test.mm`

**Step 1: Write the failing test**

Create a standalone test that:
- builds `ViewController.mm` with a stub `HJMusicPlayer`
- creates `res/c58733ac51124fe38cdc6540a7b8fa46.mkv` under the test executable resource directory
- calls `openDefault`
- fails when the captured `openURL:` argument is `nil` or empty

**Step 2: Run test to verify it fails**

Run:

```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -fmodules -Werror \
  -Iexamples/ios/HJPlayerDemo \
  -Isrc/entry/player/isys \
  examples/ios/HJPlayerDemo/ViewController_test.mm \
  examples/ios/HJPlayerDemo/ViewController.mm \
  -framework Foundation -framework UIKit \
  -o /tmp/hjplayerdemo_viewcontroller_test && /tmp/hjplayerdemo_viewcontroller_test
```

Expected:
- executable exits non-zero because `openDefault` currently calls `openURL:nil`

### Task 2: Minimal Fix

**Files:**
- Modify: `examples/ios/HJPlayerDemo/ViewController.mm`
- Test: `examples/ios/HJPlayerDemo/ViewController_test.mm`

**Step 1: Write minimal implementation**

In `ViewController.mm`:
- add a private helper that resolves `res/c58733ac51124fe38cdc6540a7b8fa46.mkv` from the bundle
- change `openDefault` to call `openURL:` with the resolved path
- if the resource cannot be found, update the state label and return without calling `openURL:nil`

**Step 2: Run test to verify it passes**

Run:

```bash
xcrun --sdk iphonesimulator clang++ -fobjc-arc -fmodules -Werror \
  -Iexamples/ios/HJPlayerDemo \
  -Isrc/entry/player/isys \
  examples/ios/HJPlayerDemo/ViewController_test.mm \
  examples/ios/HJPlayerDemo/ViewController.mm \
  -framework Foundation -framework UIKit \
  -o /tmp/hjplayerdemo_viewcontroller_test && /tmp/hjplayerdemo_viewcontroller_test
```

Expected:
- executable exits `0`
- captured `openURL:` argument is a non-empty path ending in `c58733ac51124fe38cdc6540a7b8fa46.mkv`
