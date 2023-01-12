#ifdef XR_USE_PLATFORM_ANDROID
#include "xrfw.h"
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <openxr/openxr_platform.h>

#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/AndroidAppender.h>

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
#endif