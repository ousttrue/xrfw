# xrfw

[OpenXR](https://www.khronos.org/openxr/) をラップして Windows と Android の初期化, MainLoop を共通化する小さいライブラリ。
PC は `meson` で、 Android は gradle から使えるように `cmake` でビルドする。

## Feature

* [ ] initialize OpenGL(Windows) `XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR`
* [ ] initialize OpenGLES(Android) `XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR`

## 参考

- https://github.com/KhronosGroup/OpenXR-SDK-Source
  - hello_xr
- https://developer.oculus.com/documentation/native/android/mobile-intro/
  - XrApp
