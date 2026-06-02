# HJMediaUI (iOS)

This sample demonstrates how to host an OpenGL ES 3 renderer inside a UIKit view backed by `CAEAGLLayer`, upload a texture through the `HJOGCommon` helpers, and link directly against the shared `HJRender` runtime. The app now opens to a simple home menu with buttons to enter different demos (Faceu effects or plain image sequence playback) instead of jumping straight into a single page.

## Building

1. Configure the iOS toolchain, for example:
   ```bash
   cmake -S . -B build/ios -GXcode \\
         -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \\
         -DPLATFORM=OS64COMBINED
   ```
2. Open `build/ios/HJMedia.xcodeproj` in Xcode and select the `HJMediaUI` scheme.
3. Deploy to a simulator or device running iOS 13 or later.

The target automatically copies the sample image and links `HJRender`, so no additional steps are required to see the rendered result.
