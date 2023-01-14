# xrfw

`c++20`

[OpenXR](https://www.khronos.org/openxr/) をラップして Windows と Android の初期化, MainLoop を共通化する小さいライブラリ。
PC は `meson` で、 Android は gradle から使えるように `cmake` でビルドする。

## Feature

|platform|graphics|||
|-|-|-|-|
|Windows|D3D11|||
|Windows|OpenGL|XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR|✅|
|Windows|Vulkan|||
|Android|OpenGL ES| XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR|✅|
|Android|Vulkan|||

## openxr_loader

- https://github.com/KhronosGroup/OpenXR-SDK-Source
  - xrfw_sample
- https://developer.oculus.com/documentation/native/android/mobile-intro/
  - XrApp

## samples port from

- https://github.com/terryky/android_openxr_gles
- https://github.com/microsoft/OpenXR-MixedReality/tree/main/samples/BasicXrApp
