# xrfw

`c++20`

[OpenXR](https://www.khronos.org/openxr/) をラップして OpenXR の共通処理と
シーン描画を切り分ける。

- OpenXR Instance初期化
- Graphics初期化
- OpenXR Session初期化
- OpenXR Swapchain初期化

mainloop

- Platform EventHandling
- OpenXR EventHandling

Windows は `meson` で、 Android は gradle から使えるように `cmake` でビルドする。

## Feature

|platform|graphics||
|-|-|-|
|XR_USE_PLATFORM_WIN32|XR_USE_GRAPHICS_API_D3D11|✅ VPRT work|
|XR_USE_PLATFORM_WIN32|XR_USE_GRAPHICS_API_OPENGL|✅ glfw + glew|
|XR_USE_PLATFORM_WIN32|XR_USE_GRAPHICS_API_VULKAN||
|XR_USE_PLATFORM_ANDROID|XR_USE_GRAPHICS_API_OPENGL_ES|✅ ndk|
|XR_USE_PLATFORM_ANDROID|XR_USE_GRAPHICS_API_VULKAN||

## openxr_loader

- https://github.com/KhronosGroup/OpenXR-SDK-Source
- https://developer.oculus.com/documentation/native/android/mobile-intro/

## samples port from

- https://github.com/terryky/android_openxr_gles
- https://github.com/microsoft/OpenXR-MixedReality/tree/main/samples/BasicXrApp
