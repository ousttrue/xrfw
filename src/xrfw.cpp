#define PLOG_EXPORT
#include <iomanip>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Init.h>
#include <plog/Log.h>

namespace plog {
class MyFormatter {
public:
  static util::nstring header() // This method returns a header for a new file.
                                // In our case it is empty.
  {
    return util::nstring();
  }

  static util::nstring
  format(const Record &record) // This method returns a string from a record.
  {
    tm t;
    util::localtime_s(&t, &record.getTime().time);

    util::nostringstream ss;
    ss << "["
       // hour
       << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour
       << PLOG_NSTR(":")
       // min
       << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min
       << PLOG_NSTR(".")
       // sec
       << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_sec
       << "]"
       // message
       << record.getFunc() << ": " << record.getMessage() << "\n";

    return ss.str();
  }
};
} // namespace plog

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <xrfw.h>

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;
static const int NUM_MULTI_SAMPLES = 4;
static const int MAX_NUM_EYES = 2;
static const int MAX_NUM_LAYERS = 16;

XrInstance g_instance = nullptr;
XrSystemId g_systemId = {};
XrSession g_session = nullptr;

XRFW_API int xrfwCreateInstance(const char **extensionNames,
                                uint32_t extensionCount) {

  static plog::ColorConsoleAppender<plog::MyFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);

  // instance
  XrApplicationInfo appInfo{
      .applicationName = "xrfw_sample",
      .applicationVersion = 0,
      .engineName = "xrfw_sample_engine",
      .engineVersion = 0,
      .apiVersion = XR_CURRENT_API_VERSION,
  };

  XrInstanceCreateInfo instanceCreateInfo{
      .type = XR_TYPE_INSTANCE_CREATE_INFO,
      .next = nullptr,
      .createFlags = 0,
      .applicationInfo = appInfo,
      .enabledApiLayerCount = 0,
      .enabledApiLayerNames = nullptr,
      .enabledExtensionCount = extensionCount,
      .enabledExtensionNames = extensionNames,
  };

  auto result = xrCreateInstance(&instanceCreateInfo, &g_instance);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrCreateInstance:" << result;
    return false;
  }

  XrInstanceProperties instanceInfo{
      .type = XR_TYPE_INSTANCE_PROPERTIES,
      .next = nullptr,
  };
  result = xrGetInstanceProperties(g_instance, &instanceInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProperties:" << result;
    return false;
  }
  PLOG_INFO << "Runtime " << instanceInfo.runtimeName
            << ": Version : " << XR_VERSION_MAJOR(instanceInfo.runtimeVersion)
            << XR_VERSION_MINOR(instanceInfo.runtimeVersion)
            << XR_VERSION_PATCH(instanceInfo.runtimeVersion);

  // system
  XrSystemGetInfo systemGetInfo{
      .type = XR_TYPE_SYSTEM_GET_INFO,
      .next = nullptr,
      .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
  };

  result = xrGetSystem(g_instance, &systemGetInfo, &g_systemId);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetSystem";
    return false;
  }

  XrSystemProperties systemProperties = {
      .type = XR_TYPE_SYSTEM_PROPERTIES,
  };
  result = xrGetSystemProperties(g_instance, g_systemId, &systemProperties);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetSystem";
    return false;
  }
  PLOG_INFO << "System Properties: Name=" << systemProperties.systemName
            << " VendorId=" << systemProperties.vendorId;

  PLOG_INFO << "System Graphics Properties: MaxWidth="
            << systemProperties.graphicsProperties.maxSwapchainImageWidth
            << " MaxHeight="
            << systemProperties.graphicsProperties.maxSwapchainImageHeight
            << " MaxLayers="
            << systemProperties.graphicsProperties.maxLayerCount;
  PLOG_INFO << "System Tracking Properties: OrientationTracking="
            << (systemProperties.trackingProperties.orientationTracking
                    ? "True"
                    : "False")
            << " PositionTracking="
            << (systemProperties.trackingProperties.positionTracking ? "True"
                                                                     : "False");
  if (MAX_NUM_LAYERS > systemProperties.graphicsProperties.maxLayerCount) {
    PLOG_FATAL << "systemProperties.graphicsProperties.maxLayerCount "
               << MAX_NUM_LAYERS;
    return false;
  }

  // graphics
  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR =
      nullptr;
  result = xrGetInstanceProcAddr(
      g_instance, "xrGetOpenGLGraphicsRequirementsKHR",
      (PFN_xrVoidFunction *)(&pfnGetOpenGLGraphicsRequirementsKHR));
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProcAddr: xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  XrGraphicsRequirementsOpenGLKHR graphicsRequirements = {
      .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
  };
  result = pfnGetOpenGLGraphicsRequirementsKHR(g_instance, g_systemId,
                                               &graphicsRequirements);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  return true;
}

XRFW_API void xrfwDestroyInstance() { xrDestroyInstance(g_instance); }

XRFW_API XrSession xrfwCreateOpenGLWin32Session(HDC hDC, HGLRC hGLRC) {

  XrGraphicsBindingOpenGLWin32KHR graphicsBindingGL = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
      .next = nullptr,
      .hDC = hDC,
      .hGLRC = hGLRC,
  };

  XrSessionCreateInfo sessionCreateInfo = {
      .type = XR_TYPE_SESSION_CREATE_INFO,
      .next = &graphicsBindingGL,
      .createFlags = 0,
      .systemId = g_systemId,
  };

  auto result = xrCreateSession(g_instance, &sessionCreateInfo, &g_session);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrCreateSession: " << result;
    return nullptr;
  }

  return g_session;
}

XRFW_API void xrfwDestroySession(void *session) {
  assert(session == g_session);
  xrDestroySession((XrSession)session);
}
