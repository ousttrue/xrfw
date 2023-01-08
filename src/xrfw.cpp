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
XrViewConfigurationProperties g_viewportConfig;

XRFW_API XrInstance xrfwGetInstance() { return g_instance; }

XRFW_API XrInstance xrfwCreateInstance(const char **extensionNames,
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
    return nullptr;
  }

  XrInstanceProperties instanceInfo{
      .type = XR_TYPE_INSTANCE_PROPERTIES,
      .next = nullptr,
  };
  result = xrGetInstanceProperties(g_instance, &instanceInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProperties:" << result;
    return nullptr;
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
    return nullptr;
  }

  XrSystemProperties systemProperties = {
      .type = XR_TYPE_SYSTEM_PROPERTIES,
  };
  result = xrGetSystemProperties(g_instance, g_systemId, &systemProperties);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetSystem";
    return nullptr;
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
    return nullptr;
  }

  // graphics
  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR =
      nullptr;
  result = xrGetInstanceProcAddr(
      g_instance, "xrGetOpenGLGraphicsRequirementsKHR",
      (PFN_xrVoidFunction *)(&pfnGetOpenGLGraphicsRequirementsKHR));
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProcAddr: xrGetOpenGLGraphicsRequirementsKHR";
    return nullptr;
  }

  XrGraphicsRequirementsOpenGLKHR graphicsRequirements = {
      .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
  };
  result = pfnGetOpenGLGraphicsRequirementsKHR(g_instance, g_systemId,
                                               &graphicsRequirements);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetOpenGLGraphicsRequirementsKHR";
    return nullptr;
  }

  return g_instance;
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

  // App only supports the primary stereo view config.
  const XrViewConfigurationType supportedViewConfigType =
      XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  // viewports
  {
    // Enumerate the viewport configurations.
    uint32_t viewportConfigTypeCount = 0;
    auto result = xrEnumerateViewConfigurations(g_instance, g_systemId, 0,
                                                &viewportConfigTypeCount, NULL);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrEnumerateViewConfigurations: " << result;
      return nullptr;
    }

    std::vector<XrViewConfigurationType> viewportConfigurationTypes(
        viewportConfigTypeCount);
    result = xrEnumerateViewConfigurations(
        g_instance, g_systemId, viewportConfigTypeCount,
        &viewportConfigTypeCount, viewportConfigurationTypes.data());
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrEnumerateViewConfigurations: " << result;
      return nullptr;
    }

    PLOG_INFO << "Available Viewport Configuration Types: "
              << viewportConfigTypeCount;
    for (uint32_t i = 0; i < viewportConfigTypeCount; i++) {
      const XrViewConfigurationType viewportConfigType =
          viewportConfigurationTypes[i];
      XrViewConfigurationProperties viewportConfig{
          .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES,
      };
      result = xrGetViewConfigurationProperties(
          g_instance, g_systemId, viewportConfigType, &viewportConfig);
      if (XR_FAILED(result)) {
        PLOG_FATAL << "xrGetViewConfigurationProperties: " << result;
        return nullptr;
      }
      PLOG_INFO << "  [" << i << "]FovMutable="
                << (viewportConfig.fovMutable ? "true" : "false")
                << " ConfigurationType " << viewportConfig.viewConfigurationType
                << (viewportConfigType == supportedViewConfigType ? " Selected"
                                                                  : "");

      uint32_t viewCount;
      result = xrEnumerateViewConfigurationViews(
          g_instance, g_systemId, viewportConfigType, 0, &viewCount, NULL);
      if (XR_FAILED(result)) {
        PLOG_FATAL << "xrEnumerateViewConfigurationViews: " << result;
        return nullptr;
      }
      if (viewCount > 0) {
        std::vector<XrViewConfigurationView> elements(
            viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});

        result = xrEnumerateViewConfigurationViews(
            g_instance, g_systemId, viewportConfigType, viewCount, &viewCount,
            elements.data());
        if (XR_FAILED(result)) {
          PLOG_FATAL << "xrEnumerateViewConfigurationViews: " << result;
          return nullptr;
        }
        // Log the view config info for each view type for debugging purposes.
        for (uint32_t e = 0; e < viewCount; e++) {
          const XrViewConfigurationView *element = &elements[e];

          PLOG_INFO << "    [" << i << "]"
                    << "Viewport [" << e << "]: Recommended Width="
                    << element->recommendedImageRectWidth
                    << " Height=" << element->recommendedImageRectHeight
                    << " SampleCount = "
                    << element->recommendedSwapchainSampleCount;

          PLOG_INFO << "    [" << i << "]"
                    << "Viewport [" << e
                    << "]: Max Width=" << element->maxImageRectWidth
                    << " Height=" << element->maxImageRectHeight
                    << " SampleCount = " << element->maxSwapchainSampleCount;
        }

        // Cache the view config properties for the selected config type.
        if (viewportConfigType == supportedViewConfigType) {
          assert(viewCount == MAX_NUM_EYES);
          // g_viewConfigurationView = elements;
        }
      } else {
        PLOG_ERROR << "    [" << i << "]"
                   << "Empty viewport configuration type: " << viewCount;
      }
    }
  }

  // Get the viewport configuration info for the chosen viewport configuration
  // type.
  g_viewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
  result = xrGetViewConfigurationProperties(
      g_instance, g_systemId, supportedViewConfigType, &g_viewportConfig);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetViewConfigurationProperties: " << result;
    return nullptr;
  }

  assert(g_viewportConfig.viewConfigurationType == supportedViewConfigType);
  return g_session;
}

XRFW_API void xrfwDestroySession(void *session) {
  assert(session == g_session);
  xrDestroySession((XrSession)session);
}
