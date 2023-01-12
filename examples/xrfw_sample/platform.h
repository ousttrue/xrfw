#pragma once
#include "oglrenderer.h"
#include <chrono>
#include <openxr/openxr.h>

struct Platform {
  struct PlatformImpl *impl_ = nullptr;
  Platform();
  ~Platform();
  bool InitializeGraphics();
  bool BeginFrame();
  void EndFrame(OglRenderer &renderer);
  uint32_t CastTexture(const XrSwapchainImageBaseHeader *swapchainImage);
  void Sleep(std::chrono::milliseconds ms);
  const char *const *Extensions() const;
  const void *GraphicsBinding()const;
};
