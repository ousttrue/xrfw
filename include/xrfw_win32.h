#pragma once
#include "xrfw.h"
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <winrt/base.h>

#include <openxr/openxr_platform.h>

#ifdef XR_USE_GRAPHICS_API_OPENGL
XRFW_API void xrfwInitExtensionsWin32OpenGL(
    XrGraphicsRequirementsOpenGLKHR *graphicsRequirements);
XRFW_API XrSession xrfwCreateSessionWin32OpenGL(XrfwSwapchains *swapchains,
                                                HDC dc, HGLRC glrc);
XRFW_API uint32_t
xrfwCastTextureWin32OpenGL(const XrSwapchainImageBaseHeader *swapchainImage);

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

#ifdef XR_USE_GRAPHICS_API_D3D11
XRFW_API void xrfwInitExtensionsWin32D3D11(
    XrGraphicsRequirementsD3D11KHR *graphicsRequirements);
XRFW_API XrSession xrfwCreateSessionWin32D3D11(XrfwSwapchains *swapchains,
                                               ID3D11Device *device);
XRFW_API ID3D11Texture2D *
xrfwCastTextureD3D11(const XrSwapchainImageBaseHeader *swapchainImage);

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
#endif
