#pragma once
#include "oglrenderer.h"
#include <chrono>
#include <openxr/openxr.h>
#include <span>

struct Platform {
  struct PlatformImpl *impl_ = nullptr;
  Platform(struct android_app *state = nullptr);
  ~Platform();
  bool InitializeGraphics();
  XrSession CreateSession(struct XrfwSwapchains *swapchains);
  bool BeginFrame();
  void EndFrame(OglRenderer &renderer);
  uint32_t CastTexture(const XrSwapchainImageBaseHeader *swapchainImage);
  void Sleep(std::chrono::milliseconds ms);
};
