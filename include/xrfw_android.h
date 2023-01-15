#pragma once
#include "xrfw.h"

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <openxr/openxr_platform.h>

XRFW_API XrBool32 xrfwInitializeLoaderAndroid(struct android_app *state);

#if XR_USE_GRAPHICS_API_OPENGL_ES
XRFW_API void xrfwInitExtensionsAndroidOpenGLES(
    XrGraphicsRequirementsOpenGLESKHR *graphicsRequirements,
    struct android_app *state);
XRFW_API XrSession xrfwCreateSessionAndroidOpenGLES(XrfwSwapchains *swapchains,
                                                    EGLDisplay display,
                                                    EGLContext context);

struct XrfwPlatformAndroidOpenGLES {
  struct PlatformImpl *impl_ = nullptr;
  XrfwPlatformAndroidOpenGLES(struct android_app *state = nullptr);
  ~XrfwPlatformAndroidOpenGLES();
  bool InitializeLoader();
  XrInstance CreateInstance();
  bool InitializeGraphics();
  XrSession CreateSession(struct XrfwSwapchains *swapchains);
  bool BeginFrame();
  void EndFrame(RenderFunc render, void *user);
  uint32_t CastTexture(const XrSwapchainImageBaseHeader *swapchainImage);
  void Sleep(std::chrono::milliseconds ms);
};
#endif
