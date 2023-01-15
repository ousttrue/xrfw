#ifdef XR_USE_PLATFORM_WIN32
#include "xrfw.h"
#include "xrfw_initialization.h"
#include "xrfw_plog_formatter.h"
#include "xrfw_swapchain_imagelist.h"

#ifdef XR_USE_GRAPHICS_API_D3D11
#include <d3d11.h>
#endif
#include <openxr/openxr_platform.h>

#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Init.h>
#include <plog/Log.h>

#include <algorithm>
#include <list>
#include <span>
#include <unordered_map>
#include <vector>

XRFW_API void xrfwInitLogger() {
  static plog::ColorConsoleAppender<plog::MyFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);
}

extern XrfwInitialization g_init;

#ifdef XR_USE_GRAPHICS_API_OPENGL
using SwapchainImageListOpenGL =
    SwapchainImageList<XrSwapchainImageOpenGLKHR,
                       XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR>;
template <>
std::list<std::vector<XrSwapchainImageOpenGLKHR>>
    SwapchainImageListOpenGL::s_swapchainImageBuffers = {};

static const char *opengl_extensions[1] = {
    XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
};
static bool graphicsRequirementsWin32OpenGL(XrInstance instance,
                                            XrSystemId systemId,
                                            void *outGraphicsRequirements) {

  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR =
      nullptr;
  auto result = xrGetInstanceProcAddr(
      instance, "xrGetOpenGLGraphicsRequirementsKHR",
      (PFN_xrVoidFunction *)(&pfnGetOpenGLGraphicsRequirementsKHR));
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProcAddr: xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  auto graphicsRequirements =
      (XrGraphicsRequirementsOpenGLKHR *)outGraphicsRequirements;
  graphicsRequirements->type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
  result = pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId,
                                               graphicsRequirements);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  return true;
}

static int64_t
selectColorSwapchainFormatWin32OpenGL(std::span<int64_t> swapchainFormats) {

  // List of supported color swapchain formats.
  constexpr int64_t SupportedColorSwapchainFormats[] = {
      0x8059, // GL_RGB10_A2,
      0x1908, // GL_RGBA16F,
      // The two below should only be used as a fallback, as they are linear
      // color formats without enough bits for color depth, thus leading to
      // banding.
      0x8058, // GL_RGBA8,
      0x8F97, // GL_RGBA8_SNORM,
  };

  auto swapchainFormatIt =
      std::find_first_of(swapchainFormats.begin(), swapchainFormats.end(),
                         std::begin(SupportedColorSwapchainFormats),
                         std::end(SupportedColorSwapchainFormats));
  if (swapchainFormatIt == swapchainFormats.end()) {
    throw std::runtime_error(
        "No runtime swapchain format supported for color swapchain");
  }
  return *swapchainFormatIt;
}

XRFW_API void xrfwInitExtensionsWin32OpenGL(
    XrGraphicsRequirementsOpenGLKHR *graphicsRequirements) {
  g_init.extensionNames = opengl_extensions;
  g_init.extensionCount = _countof(opengl_extensions);
  g_init.graphicsRequirements = graphicsRequirements;
  g_init.graphicsRequirementsCallback = &graphicsRequirementsWin32OpenGL;
  g_init.selectColorSwapchainFormatCallback =
      &selectColorSwapchainFormatWin32OpenGL;
  g_init.allocateSwapchainImageStructsCallback =
      &SwapchainImageListOpenGL::allocateSwapchainImageStructs;
}

XRFW_API XrSession xrfwCreateSessionWin32OpenGL(XrfwSwapchains *swapchains,
                                                HDC dc, HGLRC glrc) {
  XrGraphicsBindingOpenGLWin32KHR graphicsBinding = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
      .next = nullptr,
      .hDC = dc,
      .hGLRC = glrc,
  };
  return xrfwCreateSession(swapchains, &graphicsBinding, false);
}

XRFW_API uint32_t
xrfwCastTextureWin32OpenGL(const XrSwapchainImageBaseHeader *swapchainImage) {
  if (!swapchainImage) {
    return {};
  }
  return reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(swapchainImage)
      ->image;
}

#endif

#ifdef XR_USE_GRAPHICS_API_D3D11
using SwapchainImageListD3D11 =
    SwapchainImageList<XrSwapchainImageD3D11KHR,
                       XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR>;
template <>
std::list<std::vector<XrSwapchainImageD3D11KHR>>
    SwapchainImageListD3D11::s_swapchainImageBuffers = {};
static const char *d3d11_extensions[1] = {
    XR_KHR_D3D11_ENABLE_EXTENSION_NAME,
};
static bool graphicsRequirementsWin32D3D11(XrInstance instance,
                                           XrSystemId systemId,
                                           void *outGraphicsRequirements) {

  PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR =
      nullptr;
  auto result = xrGetInstanceProcAddr(
      instance, "xrGetD3D11GraphicsRequirementsKHR",
      (PFN_xrVoidFunction *)(&pfnGetD3D11GraphicsRequirementsKHR));
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProcAddr: xrGetD3D11GraphicsRequirementsKHR";
    return false;
  }

  auto graphicsRequirements =
      (XrGraphicsRequirementsD3D11KHR *)outGraphicsRequirements;
  graphicsRequirements->type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR;
  result = pfnGetD3D11GraphicsRequirementsKHR(instance, systemId,
                                              graphicsRequirements);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetD3D11GraphicsRequirementsKHR";
    return false;
  }

  return true;
}

static int64_t
selectColorSwapchainFormatD3D11(std::span<int64_t> swapchainFormats) {
  const static std::vector<DXGI_FORMAT> SupportedColorFormats = {
      DXGI_FORMAT_R8G8B8A8_UNORM,
      DXGI_FORMAT_B8G8R8A8_UNORM,
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
  };
  for (auto format : swapchainFormats) {
    auto found = std::find(SupportedColorFormats.begin(),
                           SupportedColorFormats.end(), format);
    if (found != SupportedColorFormats.end()) {
      return format;
    }
  }
  throw std::runtime_error(
      "No runtime swapchain format supported for color swapchain");
}

// const std::vector<DXGI_FORMAT> &SupportedDepthFormats() const override {
//   const static std::vector<DXGI_FORMAT> SupportedDepthFormats = {
//       DXGI_FORMAT_D32_FLOAT,
//       DXGI_FORMAT_D16_UNORM,
//       DXGI_FORMAT_D24_UNORM_S8_UINT,
//       DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
//   };
//   return SupportedDepthFormats;
// }

XRFW_API void xrfwInitExtensionsWin32D3D11(
    XrGraphicsRequirementsD3D11KHR *graphicsRequirements) {
  g_init.extensionNames = d3d11_extensions;
  g_init.extensionCount = _countof(d3d11_extensions);
  g_init.graphicsRequirements = graphicsRequirements;
  g_init.graphicsRequirementsCallback = &graphicsRequirementsWin32D3D11;
  g_init.selectColorSwapchainFormatCallback = &selectColorSwapchainFormatD3D11;
  g_init.allocateSwapchainImageStructsCallback =
      &SwapchainImageListD3D11::allocateSwapchainImageStructs;
}

XRFW_API XrSession xrfwCreateSessionWin32D3D11(XrfwSwapchains *swapchains,
                                               ID3D11Device *device) {
  XrGraphicsBindingD3D11KHR graphicsBinding = {
      .type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR,
      .next = nullptr,
      .device = device,
  };
  return xrfwCreateSession(swapchains, &graphicsBinding, true);
}

ID3D11Texture2D *
xrfwCastTextureD3D11(const XrSwapchainImageBaseHeader *swapchainImage) {
  if (!swapchainImage) {
    return {};
  }
  return reinterpret_cast<const XrSwapchainImageD3D11KHR *>(swapchainImage)
      ->texture;
}

#endif
#endif
