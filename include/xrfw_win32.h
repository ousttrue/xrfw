#pragma once
#include "xrfw.h"
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#if XR_USE_GRAPHICS_API_OPENGL
XRFW_API void xrfwPlatformWin32OpenGL(XrfwInitialization *init);
XRFW_API XrSession xrfwCreateSessionWin32OpenGL(XrfwSwapchains *swapchains,
                                                HDC dc, HGLRC glrc);

struct XrfwPlatformWin32OpenGL {
  struct PlatformWin32OpenGLImpl *impl_ = nullptr;
  XrfwPlatformWin32OpenGL(struct android_app *state = nullptr);
  ~XrfwPlatformWin32OpenGL();
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

#if XR_USE_GRAPHICS_API_D3D11
#include <d3d11.h>
XRFW_API void xrfwPlatformWin32D3D11(XrfwInitialization *init);
XRFW_API XrSession xrfwCreateSessionWin32D3D11(XrfwSwapchains *swapchains);

struct XrfwPlatformWin32D3D11 {
  struct PlatformWin32D3D11Impl *impl_ = nullptr;
  XrfwPlatformWin32D3D11(struct android_app *state = nullptr);
  ~XrfwPlatformWin32D3D11();
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
