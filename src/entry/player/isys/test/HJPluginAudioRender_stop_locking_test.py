#!/usr/bin/env python3

import pathlib
import sys


def extract_method_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise AssertionError(f"Missing method signature: {signature}")

    brace_start = source.find("{", start)
    if brace_start < 0:
        raise AssertionError(f"Missing body start for: {signature}")

    depth = 0
    for index in range(brace_start, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace_start + 1:index]

    raise AssertionError(f"Missing body end for: {signature}")


def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parents[5]
    source = (repo_root / "src/plugins/HJPluginAudioRender.cpp").read_text(encoding="utf-8")

    run_reinit = extract_method_body(source, "int HJPluginAudioRender::runReinit()")
    run_run_op = extract_method_body(source, "void HJPluginAudioRender::runRunOp(RunOp op)")

    required_snippets = [
        "releaseRender();",
        "clearPCMCallbackData(0);",
        "ret = initRender(audioInfo);",
        "ret = setStreamRunning(true);",
        "ret = setStreamRunning(running, eofStop);",
        "RemoteIO stop can wait for its callback thread",
    ]
    for snippet in required_snippets:
        if snippet not in source:
            print(f"FAIL: missing snippet: {snippet}", file=sys.stderr)
            return 1

    forbidden_reinit = [
        "releaseRender();\n        clearPCMCallbackData(0);\n\n        auto err = initRender(audioInfo);",
        "err = setStreamRunning(true);",
    ]
    for snippet in forbidden_reinit:
        if snippet in run_reinit:
            print(f"FAIL: runReinit still performs long AudioUnit work inside state-lock block: {snippet}", file=sys.stderr)
            return 1

    forbidden_run_op = [
        "return setStreamRunning(false, false);",
        "return setStreamRunning(false, true);",
        "if (setStreamRunning(true) != HJ_OK) {",
    ]
    for snippet in forbidden_run_op:
        if snippet in run_run_op:
            print(f"FAIL: runRunOp still calls setStreamRunning directly in the SYNC_PROD_LOCK path: {snippet}", file=sys.stderr)
            return 1

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
