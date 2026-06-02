# iOS FaceSR Rect-Only Face Detection v1 Delivery Notes

## Implementation Items

- Add new public and internal backend enums.
- Add `HJFaceDetectIOSVisionRect` iOS backend.
- Update factory and wrapper routing.
- Update iOS CMake framework linkage.
- Add canonical product, architecture, and delivery docs.

## Verification Targets

- iOS build for `HJDetect` and `HJInference`.
- `NV12` input returns in-bounds source-frame pixel rects.
- `RGB` input returns in-bounds source-frame pixel rects.
- No-face path remains non-fatal.
- Multi-face output remains deterministic enough for first-face crop selection.
- `FaceSR` crops using returned `faceRect` without depending on landmarks.

## Known Limits

- No 5-point output in v1.
- Vision boxes are accepted as same-coordinate-system results, not model-matched results.
- Landmark-driven paths must not switch to this backend unless they are updated to tolerate rect-only output.
