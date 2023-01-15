#pragma once
#include "xrfw_render_func.h"
#include <chrono>
#include <d3d11.h>
#include <winrt/base.h>

struct XrfwPlatformWin32D3D11 {
  struct PlatformWin32D3D11Impl *impl_ = nullptr;
  XrfwPlatformWin32D3D11(struct android_app *state = nullptr);
  ~XrfwPlatformWin32D3D11();
  bool InitializeLoader();
  XrInstance CreateInstance();
  winrt::com_ptr<ID3D11Device> InitializeGraphics();
  XrSession CreateSession(struct XrfwSwapchains *swapchains);
  bool BeginFrame();
  void EndFrame(RenderFunc render, void *user);
  void Sleep(std::chrono::milliseconds ms);
};
