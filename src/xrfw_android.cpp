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

XRFW_API XrBool32 xrfwInitializeLoaderAndroid(struct android_app *state) {
  //
  // Initialize the loader for this platform
  //
  PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
  if (XR_FAILED(
          xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                (PFN_xrVoidFunction *)(&initializeLoader)))) {
    PLOG_FATAL << "no xrInitializeLoaderKHR";
    return false;
  }

  XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid{
      .type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
      .next = NULL,
      .applicationVM = state->activity->vm,
      .applicationContext = state->activity->clazz,
  };
  if (XR_FAILED(initializeLoader(
          (const XrLoaderInitInfoBaseHeaderKHR *)&loaderInitInfoAndroid))) {
    PLOG_FATAL << "xrInitializeLoaderKHR";
    return false;
  }
  return true;
}

XRFW_API void xrfwInitLogger() {
  static plog::AndroidAppender<plog::TxtFormatter> androidAppender("app");
  plog::init(plog::verbose, &androidAppender);
}

static const char *extensions[2] = {
    XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
    XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
};
XrInstanceCreateInfoAndroidKHR g_instanceCreateInfoAndroid = {};

XRFW_API void xrfwPlatformAndroidOpenGLES(XrfwInitialization *init,
                                          android_app *state) {
  g_instanceCreateInfoAndroid = {
      .type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
      .applicationVM = state->activity->vm,
      .applicationActivity = state->activity->clazz,
  };

  init->extensionNames = extensions;
  init->extensionCount = sizeof(extensions) / sizeof(extensions[0]);
  init->next = &g_instanceCreateInfoAndroid;
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

XRFW_API XrSession xrfwCreateSessionAndroidOpenGLES(XrfwSwapchains *swapchains,
                                                    EGLDisplay display,
                                                    EGLContext context) {
  XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
      .display = display,
      .context = context,
  };
  return xrfwCreateSession(swapchains, &graphicsBinding);
}
#endif
