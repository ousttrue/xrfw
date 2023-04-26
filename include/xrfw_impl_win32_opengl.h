#pragma once

#include "xrfw_func.h"
#include <chrono>

struct XrfwPlatformWin32OpenGL
{
  struct PlatformWin32OpenGLImpl* impl_ = nullptr;
  XrfwPlatformWin32OpenGL(struct android_app* state = nullptr);
  ~XrfwPlatformWin32OpenGL();
  bool InitializeLoader();
  XrInstance CreateInstance();
  bool InitializeGraphics();
  XrSession CreateSession(struct XrfwSwapchains* swapchains);
  bool BeginFrame();
  void EndFrame(RenderFunc render, void* user);
  uint32_t CastTexture(const XrSwapchainImageBaseHeader* swapchainImage);
  void Sleep(std::chrono::milliseconds ms);
};
