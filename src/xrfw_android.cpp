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

#include <list>

void _xrfwInitLogger() {
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

  // Initialize the gl extensions. Note we have to open a window.
  // ksDriverInstance driverInstance{};
  // ksGpuQueueInfo queueInfo{};
  // ksGpuSurfaceColorFormat colorFormat{KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8};
  // ksGpuSurfaceDepthFormat depthFormat{KS_GPU_SURFACE_DEPTH_FORMAT_D24};
  // ksGpuSampleCount sampleCount{KS_GPU_SAMPLE_COUNT_1};
  // if (!ksGpuWindow_Create(&window, &driverInstance, &queueInfo, 0,
  // colorFormat,
  //                         depthFormat, sampleCount, 640, 480, false)) {
  //   THROW("Unable to create GL context");
  // }

  // GLint major = 0;
  // GLint minor = 0;
  // glGetIntegerv(GL_MAJOR_VERSION, &major);
  // glGetIntegerv(GL_MINOR_VERSION, &minor);

  // const XrVersion desiredApiVersion = XR_MAKE_VERSION(major, minor, 0);
  // if (graphicsRequirements.minApiVersionSupported > desiredApiVersion) {
  //   THROW("Runtime does not support desired Graphics API and/or version");
  // }

  // m_contextApiMajorVersion = major;

  // #if defined(XR_USE_PLATFORM_ANDROID)
  //   m_graphicsBinding.display = window.display;
  //   m_graphicsBinding.config = (EGLConfig)0;
  //   m_graphicsBinding.context = window.context.context;
  // #endif

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