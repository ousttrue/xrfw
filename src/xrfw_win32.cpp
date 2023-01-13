#ifdef XR_USE_PLATFORM_WIN32
#include "xrfw.h"
#include "xrfw_plog_formatter.h"

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

// require openxr graphics extension
static const char *extensions[1] = {
    XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
};
XRFW_API void xrfwPlatformWin32OpenGL(XrfwInitialization *init) {
  init->extensionNames = extensions;
  init->extensionCount = _countof(extensions);
}

bool _xrfwGraphicsRequirements(XrInstance instance, XrSystemId systemId) {

  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR =
      nullptr;
  auto result = xrGetInstanceProcAddr(
      instance, "xrGetOpenGLGraphicsRequirementsKHR",
      (PFN_xrVoidFunction *)(&pfnGetOpenGLGraphicsRequirementsKHR));
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProcAddr: xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  XrGraphicsRequirementsOpenGLKHR graphicsRequirements = {
      .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
  };
  result = pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId,
                                               &graphicsRequirements);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  return true;
}

int64_t _xrfwSelectColorSwapchainFormat(std::span<int64_t> swapchainFormats) {

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

std::list<std::vector<XrSwapchainImageOpenGLKHR>> g_swapchainImageBuffers;
std::vector<XrSwapchainImageBaseHeader *> _xrfwAllocateSwapchainImageStructs(
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

XRFW_API XrSession xrfwCreateSessionWin32OpenGL(XrfwSwapchains *swapchains,
                                                HDC dc, HGLRC glrc) {
  XrGraphicsBindingOpenGLWin32KHR graphicsBindingGL = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
      .next = nullptr,
      .hDC = dc,
      .hGLRC = glrc,
  };
  return xrfwCreateSession(swapchains, &graphicsBindingGL);
}
#endif