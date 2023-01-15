#pragma once
#include "xrfw_render_func.h"
#include <chrono>

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
