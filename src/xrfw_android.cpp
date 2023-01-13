#ifdef XR_USE_PLATFORM_ANDROID
#include "xrfw.h"
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <openxr/openxr_platform.h>

#include <plog/Appenders/AndroidAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>

#include <GLES3/gl32.h>

#include <list>

XRFW_API void xrfwInitLogger() {
  static plog::AndroidAppender<plog::TxtFormatter> androidAppender("app");
  plog::init(plog::verbose, &androidAppender);
}

bool _xrfwGraphicsRequirements(XrInstance instance, XrSystemId systemId) {

  PFN_xrGetOpenGLESGraphicsRequirementsKHR
      pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
  auto result =
      xrGetInstanceProcAddr(instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                            reinterpret_cast<PFN_xrVoidFunction *>(
                                &pfnGetOpenGLESGraphicsRequirementsKHR));
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProcAddr: xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  XrGraphicsRequirementsOpenGLESKHR graphicsRequirements{
      XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
  result = pfnGetOpenGLESGraphicsRequirementsKHR(instance, systemId,
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
      GL_RGBA8, GL_RGBA8_SNORM, GL_SRGB8_ALPHA8};

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

std::list<std::vector<XrSwapchainImageOpenGLESKHR>> g_swapchainImageBuffers;
std::vector<XrSwapchainImageBaseHeader *> _xrfwAllocateSwapchainImageStructs(
    uint32_t capacity, const XrSwapchainCreateInfo & /*swapchainCreateInfo*/) {
  // Allocate and initialize the buffer of image structs (must be sequential in
  // memory for xrEnumerateSwapchainImages). Return back an array of pointers to
  // each swapchain image struct so the consumer doesn't need to know the
  // type/size.
  std::vector<XrSwapchainImageOpenGLESKHR> swapchainImageBuffer(capacity);
  std::vector<XrSwapchainImageBaseHeader *> swapchainImageBase;
  for (XrSwapchainImageOpenGLESKHR &image : swapchainImageBuffer) {
    image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
    swapchainImageBase.push_back(
        reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
  }

  // Keep the buffer alive by moving it into the list of buffers.
  g_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));

  return swapchainImageBase;
}
#endif