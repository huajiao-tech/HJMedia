#!/usr/bin/env python3

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[5]
HEADER = ROOT / "src/entry/player/isys/HJMusicPlayer.h"
IMPL = ROOT / "src/entry/player/isys/HJMusicPlayer.mm"
DEMUXER = ROOT / "src/plugins/HJPluginDemuxer.cpp"


def require(text: str, needle: str, message: str) -> None:
    if needle not in text:
        raise AssertionError(message)


def forbid(text: str, needle: str, message: str) -> None:
    if needle in text:
        raise AssertionError(message)


def main() -> int:
    header_text = HEADER.read_text(encoding="utf-8")
    impl_text = IMPL.read_text(encoding="utf-8")
    demuxer_text = DEMUXER.read_text(encoding="utf-8")

    require(
        header_text,
        "extern NSString * const HJMusicPlayerOpenOptionStartTime;",
        "HJMusicPlayer.h must expose the startTime option key",
    )
    require(
        header_text,
        "extern NSString * const HJMusicPlayerOpenOptionAudioTrackID;",
        "HJMusicPlayer.h must expose the audioTrackID option key",
    )
    require(
        header_text,
        "- (NSInteger)openURL:(nullable NSString *)url options:",
        "HJMusicPlayer.h must expose the options-based openURL API",
    )
    forbid(
        header_text,
        "- (NSInteger)openURL:(nullable NSString *)url startTime:(int64_t)startTime;",
        "HJMusicPlayer.h should no longer expose the startTime-based openURL API",
    )

    require(
        impl_text,
        "NSString * const HJMusicPlayerOpenOptionStartTime",
        "HJMusicPlayer.mm must define the startTime option key",
    )
    require(
        impl_text,
        '@"startTime";',
        "HJMusicPlayer.mm must map the startTime option key to the startTime dictionary field",
    )
    require(
        impl_text,
        "NSString * const HJMusicPlayerOpenOptionAudioTrackID",
        "HJMusicPlayer.mm must define the audioTrackID option key",
    )
    require(
        impl_text,
        '@"audioTrackID";',
        "HJMusicPlayer.mm must map the audioTrackID option key to the audioTrackID dictionary field",
    )
    require(
        impl_text,
        "id startTimeObject = options[HJMusicPlayerOpenOptionStartTime];",
        "HJMusicPlayer.mm must read startTime from the exported option key",
    )
    require(
        impl_text,
        "id trackIDObject = options[HJMusicPlayerOpenOptionAudioTrackID];",
        "HJMusicPlayer.mm must read audioTrackID from the exported option key",
    )
    require(
        impl_text,
        "if (audioTrackID >= 0) {",
        "HJMusicPlayer.mm must allow track 0 to be forwarded during openURL",
    )

    require(
        demuxer_text,
        'if (i_url->haveValue("audioTrackID")) {',
        "HJPluginDemuxer.cpp must read audioTrackID from the media URL options",
    )
    require(
        demuxer_text,
        "requestedAudioTrackID = i_url->getValue<int>(\"audioTrackID\");",
        "HJPluginDemuxer.cpp must parse the requested initial audio track ID",
    )
    require(
        demuxer_text,
        "if (ret == HJErrNotFind) {",
        "HJPluginDemuxer.cpp must ignore missing audio tracks during initial open",
    )
    require(
        demuxer_text,
        "ret = HJ_OK;",
        "HJPluginDemuxer.cpp must keep the existing demuxer selection when the requested track is missing",
    )

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(1)
