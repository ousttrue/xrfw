#ifdef XR_USE_PLATFORM_WIN32
#include "xrfw.h"
#include "xrfw_initialization.h"
#include "xrfw_plog_formatter.h"

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

XRFW_API void xrfwInitLogger() {
  static plog::ColorConsoleAppender<plog::MyFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);
}

extern XrfwInitialization g_init;

#ifdef XR_USE_GRAPHICS_API_OPENGL
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

static std::list<std::vector<XrSwapchainImageOpenGLKHR>>
    g_swapchainImageBuffers;
static std::vector<XrSwapchainImageBaseHeader *> allocateSwapchainImageStructs(
    uint32_t capacity, const XrSwapchainCreateInfo & /*swapchainCreateInfo*/) {
  // Allocate and initialize the buffer of image structs (must be sequential in
  // memory for xrEnumerateSwapchainImages). Return back an array of pointers to
  // each swapchain image struct so the consumer doesn't need to know the
  // type/size.
  std::vector<XrSwapchainImageOpenGLKHR> swapchainImageBuffer(capacity);
  std::vector<XrSwapchainImageBaseHeader *> swapchainImageBase;
  for (XrSwapchainImageOpenGLKHR &image : swapchainImageBuffer) {
    image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
    swapchainImageBase.push_back(
        reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
  }

  // Keep the buffer alive by moving it into the list of buffers.
  g_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));

  return swapchainImageBase;
}

XRFW_API void xrfwInitExtensionsWin32OpenGL(
    XrGraphicsRequirementsOpenGLKHR *graphicsRequirements) {
  g_init.extensionNames = opengl_extensions;
  g_init.extensionCount = _countof(opengl_extensions);
  g_init.graphicsRequirements = graphicsRequirements;
  g_init.graphicsRequirementsCallback = &graphicsRequirementsWin32OpenGL;
  g_init.selectColorSwapchainFormatCallback =
      &selectColorSwapchainFormatWin32OpenGL;
  g_init.allocateSwapchainImageStructsCallback = &allocateSwapchainImageStructs;
}

XRFW_API XrSession xrfwCreateSessionWin32OpenGL(XrfwSwapchains *swapchains,
                                                HDC dc, HGLRC glrc) {
  XrGraphicsBindingOpenGLWin32KHR graphicsBinding = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
      .next = nullptr,
      .hDC = dc,
      .hGLRC = glrc,
  };
  return xrfwCreateSession(swapchains, &graphicsBinding);
}

XRFW_API uint32_t
xrfwCastTextureWin32OpenGL(const XrSwapchainImageBaseHeader *swapchainImage) {
  return reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(swapchainImage)
      ->image;
}

#endif

#ifdef XR_USE_GRAPHICS_API_D3D11
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

XRFW_API void xrfwInitExtensionsWin32D3D11(
    XrGraphicsRequirementsD3D11KHR *graphicsRequirements) {
  g_init.extensionNames = d3d11_extensions;
  g_init.extensionCount = _countof(d3d11_extensions);
  g_init.graphicsRequirements = graphicsRequirements;
  g_init.graphicsRequirementsCallback = &graphicsRequirementsWin32D3D11;
  // g_init.selectColorSwapchainFormatCallback =
  //     &selectColorSwapchainFormatWin32OpenGL;
  // g_init.allocateSwapchainImageStructsCallback =
  // &allocateSwapchainImageStructs;
}

XRFW_API XrSession xrfwCreateSessionWin32D3D11(XrfwSwapchains *swapchains,
                                               ID3D11Device *device) {
  XrGraphicsBindingD3D11KHR graphicsBinding = {
      .type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR,
      .next = nullptr,
      .device = device,
  };
  return xrfwCreateSession(swapchains, &graphicsBinding);
}

ID3D11Texture2D *
xrfwCastTextureD3D11(const XrSwapchainImageBaseHeader *swapchainImage) {
  return reinterpret_cast<const XrSwapchainImageD3D11KHR *>(swapchainImage)
      ->texture;
}

#endif
#endif
