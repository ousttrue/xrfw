#ifdef XR_USE_PLATFORM_ANDROID
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include "xrfw.h"
#include "xrfw_initialization.h"
#include "xrfw_swapchain_imagelist.h"

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

using SwapchainImageListOpenGLES =
    SwapchainImageList<XrSwapchainImageOpenGLESKHR,
                       XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR>;
template <>
std::list<std::vector<XrSwapchainImageOpenGLESKHR>>
    SwapchainImageListOpenGLES::s_swapchainImageBuffers = {};

static const char *const extensions[2] = {
    XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
    XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
};
XrInstanceCreateInfoAndroidKHR g_instanceCreateInfoAndroid = {};

extern XrfwInitialization g_init;

static bool graphicsRequirementsOpenGLES(XrInstance instance,
                                         XrSystemId systemId,
                                         void *outGraphicsRequirements) {

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

  auto graphicsRequirements =
      (XrGraphicsRequirementsOpenGLESKHR *)outGraphicsRequirements;
  graphicsRequirements->type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
  result = pfnGetOpenGLESGraphicsRequirementsKHR(instance, systemId,
                                                 graphicsRequirements);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  return true;
}

static int64_t
selectColorSwapchainFormatOpenGLES(std::span<int64_t> swapchainFormats) {

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

XRFW_API void xrfwInitExtensionsAndroidOpenGLES(
    XrGraphicsRequirementsOpenGLESKHR *graphicsRequirements,
    android_app *state) {
  g_instanceCreateInfoAndroid = {
      .type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
      .applicationVM = state->activity->vm,
      .applicationActivity = state->activity->clazz,
  };

  g_init.extensionNames = extensions;
  g_init.extensionCount = std::size(extensions);
  g_init.next = &g_instanceCreateInfoAndroid;
  g_init.graphicsRequirements = graphicsRequirements;
  g_init.graphicsRequirementsCallback = &graphicsRequirementsOpenGLES;
  g_init.selectColorSwapchainFormatCallback =
      &selectColorSwapchainFormatOpenGLES;
  g_init.allocateSwapchainImageStructsCallback =
      &SwapchainImageListOpenGLES::allocateSwapchainImageStructs;
}

XRFW_API XrSession xrfwCreateSessionAndroidOpenGLES(XrfwSwapchains *swapchains,
                                                    EGLDisplay display,
                                                    EGLContext context) {
  XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
      .display = display,
      .context = context,
  };
  return xrfwCreateSession(swapchains, &graphicsBinding, false);
}
#endif
