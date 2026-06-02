# iOS FaceSR Rect-Only Face Detection v1

## Goal

Add an iOS-only face detection backend for `FaceSR` that returns face rectangles in the same source-frame pixel coordinate space used by the existing NCNN backends.

## Intent

- Use Apple `Vision` face rectangle detection for iOS.
- Support `FaceSR`, face crop, and similar rect-driven flows.
- Do not replace existing landmark-capable backends.

## Non-Goals

- No 5-point landmark compatibility in v1.
- No promise that the numeric box matches `SCRFD` or `RetinaFace`.
- No silent backend remapping on iOS.

## Success Criteria

- New explicit backend enum is available to callers.
- Returned `Rect` is `(x, y, w, h)` in original-frame pixels with top-left origin.
- `HJFaceAcquireWrapper` and `HJFaceSRWrapper` continue to crop correctly when the backend returns rects but no keypoints.
- Landmark-driven paths remain on existing NCNN/TNN backends.
