# HJMusicPlayer Error Logging Coverage Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add `HJFLoge` coverage to every meaningful `HJMusicPlayer.mm` failure path so playback issues can be traced from logs.

**Architecture:** Keep the behavior unchanged and only add diagnostics in the wrapper layer. Cover public API failures, internal prepare/audio-session failures, and ignored event-bus registration failures with explicit context-rich `HJFLoge` messages. Validate the change with a source-level regression test that searches for required error log snippets in `HJMusicPlayer.mm`.

**Tech Stack:** Objective-C++, Foundation, Python 3 source inspection test

---

### Task 1: Regression Test

**Files:**
- Create: `src/entry/player/isys/test/HJMusicPlayer_error_logging_test.py`
- Test: `src/entry/player/isys/test/HJMusicPlayer_error_logging_test.py`

**Step 1: Write the failing test**

Create a source-level regression test that asserts `HJMusicPlayer.mm` contains explicit `HJFLoge` messages for:
- `prepare`, `openURL`, `pause`, `resume`, `seekToMilliseconds`, `setMute`, `setVolume`, `selectAudioTrack`
- `hj_prepareLocked`, including player-context/createGraph/init failures
- `hj_prepareAudioSessionIfNeeded`
- failed `EVENT_GRAPH_EOF_ID` / `EVENT_GRAPH_RENDERED_PCM_ID` registration

**Step 2: Run test to verify it fails**

Run:

```bash
python3 src/entry/player/isys/test/HJMusicPlayer_error_logging_test.py
```

Expected:
- FAIL because several required `HJFLoge` snippets are still missing

### Task 2: Minimal Logging Fix

**Files:**
- Modify: `src/entry/player/isys/HJMusicPlayer.mm`
- Test: `src/entry/player/isys/test/HJMusicPlayer_error_logging_test.py`

**Step 1: Write minimal implementation**

In `HJMusicPlayer.mm`:
- add `HJFLoge` immediately before every negative return branch in public methods
- add `HJFLoge` for `hj_prepareLocked` and `hj_prepareAudioSessionIfNeeded` failure branches
- check return values from `EVENT_GRAPH_EOF_ID` and `EVENT_GRAPH_RENDERED_PCM_ID` handler registration, log failures without changing control flow

**Step 2: Run test to verify it passes**

Run:

```bash
python3 src/entry/player/isys/test/HJMusicPlayer_error_logging_test.py
```

Expected:
- PASS
