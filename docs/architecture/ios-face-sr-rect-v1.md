# iOS FaceSR Rect-Only Face Detection v1 Design

## Architecture

- Add `HJFaceDetectWrapperType_IOSVISIONRECT` and map it to `HJFaceDetectType_IOS_VISION_RECT`.
- Implement `HJFaceDetectIOSVisionRect` under `src/detect/osys/vision/`.
- Keep threading unchanged: detection runs synchronously inside `HJBaseFaceDetect::detect`; wrapper async behavior remains in `HJFaceDetectWrapper`.

## Data Flow

1. `HJFaceDetectWrapper` selects the explicit iOS Vision backend.
2. `HJFaceDetectIOSVisionRect` accepts `NV12` or `RGB` `HJTransferMediaData`.
3. Input is converted into a `CVPixelBuffer`.
4. `VNDetectFaceRectanglesRequest` runs on the pixel buffer.
5. Vision normalized lower-left bounding boxes are converted to top-left source-frame pixel rects.
6. `HJFaceDetectRet` is filled with valid face rects and empty keypoint arrays.
7. `HJBaseFaceDetect::cvtConcisePoints` serializes rect-only faces so `HJFaceAcquireWrapper` can keep cropping from `faceRect`.

## Compatibility Notes

- v1 is valid for `FaceSR`, crop, and face-protection style flows.
- v1 is not a replacement for `FaceU` or sticker/render paths that expect stable landmarks.
- Existing NCNN/TNN backends remain authoritative for landmark-driven use cases.

## Build Impact

- iOS detect and inference targets link the `Vision` framework.
