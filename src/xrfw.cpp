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
// App only supports the primary stereo view config.
static const XrViewConfigurationType g_supportedViewConfigType =
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

XrInstance g_instance = nullptr;
XrSystemId g_systemId = {};
XrSession g_session = nullptr;
// XrViewConfigurationProperties g_viewportConfig;
XrEventDataBuffer g_eventDataBuffer = {};
XrSessionState g_sessionState = XR_SESSION_STATE_UNKNOWN;
bool g_sessionRunning = false;
XrSpace g_currentSpace = {};
XrSpace g_headSpace = {};
XrSpace g_localSpace = {};

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
                << (viewportConfigType == g_supportedViewConfigType
                        ? " Selected"
                        : "");

      uint32_t viewCount;
      result = xrEnumerateViewConfigurationViews(
          g_instance, g_systemId, viewportConfigType, 0, &viewCount, NULL);
      if (XR_FAILED(result)) {
        PLOG_FATAL << "xrEnumerateViewConfigurationViews: " << result;
        return nullptr;
      }
    }
  }

  // Get the viewport configuration info for the chosen viewport configuration
  // type.
  // g_viewportConfig = {.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
  // result = xrGetViewConfigurationProperties(
  //     g_instance, g_systemId, g_supportedViewConfigType, &g_viewportConfig);
  // if (XR_FAILED(result)) {
  //   PLOG_FATAL << "xrGetViewConfigurationProperties: " << result;
  //   return nullptr;
  // }
  // assert(g_viewportConfig.viewConfigurationType ==
  // g_supportedViewConfigType);

  // base space
  {
    XrReferenceSpaceCreateInfo spaceCreateInfo = {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
        .poseInReferenceSpace =
            {
                .orientation = {0, 0, 0, 1},
                .position = {0, 0, 0},
            },
    };
    auto result =
        xrCreateReferenceSpace(g_session, &spaceCreateInfo, &g_currentSpace);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return {};
    }
  }

  // head / local
  {
    XrReferenceSpaceCreateInfo spaceCreateInfo = {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
        .poseInReferenceSpace =
            {
                .orientation = {0, 0, 0, 1},
                .position = {0, 0, 0},
            },
    };
    auto result =
        xrCreateReferenceSpace(g_session, &spaceCreateInfo, &g_headSpace);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return {};
    }
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    result = xrCreateReferenceSpace(g_session, &spaceCreateInfo, &g_localSpace);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return {};
    }
  }

  return g_session;
}

XRFW_API void xrfwDestroySession(void *session) {
  assert(session == g_session);
  xrDestroySession((XrSession)session);
}

XRFW_API XrBool32 xrfwGetViewConfigurationViews(
    XrViewConfigurationView *viewConfigurationViews, uint32_t viewCount) {

  for (int i = 0; i < viewCount; ++i) {
    viewConfigurationViews[i] = {XR_TYPE_VIEW_CONFIGURATION_VIEW};
  }
  auto result = xrEnumerateViewConfigurationViews(
      g_instance, g_systemId, g_supportedViewConfigType, viewCount, &viewCount,
      viewConfigurationViews);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrEnumerateViewConfigurationViews: " << result;
    return {};
  }

  // Log the view config info for each view type for debugging purposes.
  for (uint32_t e = 0; e < viewCount; e++) {
    const XrViewConfigurationView *element = &viewConfigurationViews[e];

    PLOG_INFO << "Viewport [" << e
              << "]: Recommended Width=" << element->recommendedImageRectWidth
              << " Height=" << element->recommendedImageRectHeight
              << " SampleCount = " << element->recommendedSwapchainSampleCount;

    PLOG_INFO << "Viewport [" << e
              << "]: Max Width=" << element->maxImageRectWidth
              << " Height=" << element->maxImageRectHeight
              << " SampleCount = " << element->maxSwapchainSampleCount;
  }

  return XR_TRUE;
}

// Return event if one is available, otherwise return null.
static const XrEventDataBaseHeader *TryReadNextEvent() {
  // It is sufficient to clear the just the XrEventDataBuffer header to
  // XR_TYPE_EVENT_DATA_BUFFER
  auto baseHeader =
      reinterpret_cast<XrEventDataBaseHeader *>(&g_eventDataBuffer);
  *baseHeader = {XR_TYPE_EVENT_DATA_BUFFER};
  auto result = xrPollEvent(g_instance, &g_eventDataBuffer);
  if (result == XR_SUCCESS) {
    if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
      const XrEventDataEventsLost *const eventsLost =
          reinterpret_cast<const XrEventDataEventsLost *>(baseHeader);
      PLOG_WARNING << eventsLost << " events lost";
    }

    return baseHeader;
  }
  if (result == XR_EVENT_UNAVAILABLE) {
    return nullptr;
  }
  PLOG_FATAL << result;
  throw std::runtime_error("xrPollEvent");
}

static void HandleSessionStateChangedEvent(
    const XrEventDataSessionStateChanged &stateChangedEvent) {
  auto oldState = g_sessionState;
  g_sessionState = stateChangedEvent.state;
  PLOG_INFO << oldState << " => " << g_sessionState
      // << " session=" << stateChangedEvent.session
      // << " time=" << stateChangedEvent.time
      ;

  if ((stateChangedEvent.session != XR_NULL_HANDLE) &&
      (stateChangedEvent.session != g_session)) {
    throw std::runtime_error(
        "XrEventDataSessionStateChanged for unknown session");
  }

  switch (g_sessionState) {

  case XR_SESSION_STATE_READY: {
    XrSessionBeginInfo sessionBeginInfo{
        .type = XR_TYPE_SESSION_BEGIN_INFO,
        .primaryViewConfigurationType =
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    };
    auto result = xrBeginSession(g_session, &sessionBeginInfo);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      throw std::runtime_error("[xrBeginSession]");
    }
    PLOG_INFO << "xrBeginSession";
    g_sessionRunning = true;
    break;
  }

  case XR_SESSION_STATE_STOPPING: {
    auto result = xrEndSession(g_session);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      throw std::runtime_error("[xrEndSession]");
    }
    PLOG_INFO << "xrEndSession";
    g_sessionRunning = false;
    break;
  }

  case XR_SESSION_STATE_EXITING: {
    // Do not attempt to restart because user closed this session.
    g_sessionRunning = false;
    break;
  }

  case XR_SESSION_STATE_LOSS_PENDING: {
    // Poll for a new instance.
    g_sessionRunning = false;
    break;
  }

  default:
    break;
  }
}

XRFW_API XrBool32 xrfwPollEventsAndIsActive() {
  // Process all pending messages.
  while (const XrEventDataBaseHeader *event = TryReadNextEvent()) {
    switch (event->type) {
    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
      const auto &instanceLossPending =
          *reinterpret_cast<const XrEventDataInstanceLossPending *>(event);
      PLOG_WARNING << "XrEventDataInstanceLossPending by "
                   << instanceLossPending.lossTime;
      break;
    }

    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
      auto sessionStateChangedEvent =
          *reinterpret_cast<const XrEventDataSessionStateChanged *>(event);
      HandleSessionStateChangedEvent(sessionStateChangedEvent);
      break;
    }

    default: {
      PLOG_INFO << "unknown event type " << event->type;
      break;
    }
    }
  }
  return g_sessionRunning;
}

XRFW_API XrBool32 xrfwBeginFrame(XrTime *outtime, XrView views[2]) {
  XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState frameState{XR_TYPE_FRAME_STATE};
  auto result = xrWaitFrame(g_session, &frameWaitInfo, &frameState);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return false;
  }
  *outtime = frameState.predictedDisplayTime;

  static XrBool32 shouldRender_ = false;
  if (shouldRender_ != frameState.shouldRender) {
    PLOG_INFO << "shouldRender: " << shouldRender_ << " => "
              << frameState.shouldRender;
    shouldRender_ = frameState.shouldRender;
  }

  XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
  result = xrBeginFrame(g_session, &frameBeginInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return false;
  }

  if (shouldRender_) {
    // view
    XrViewState viewState{XR_TYPE_VIEW_STATE};
    XrViewLocateInfo viewLocateInfo{
        .type = XR_TYPE_VIEW_LOCATE_INFO,
        .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        .displayTime = frameState.predictedDisplayTime,
        .space = g_currentSpace,
    };
    uint32_t viewCountOutput;
    auto result = xrLocateViews(g_session, &viewLocateInfo, &viewState, 2,
                                &viewCountOutput, views);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrLocateViews: " << result;
      return false;
    }
    if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
      return false; // There is no valid tracking poses for the views.
    }
    assert(viewCountOutput == 2);
  }

  return shouldRender_;
}

XRFW_API XrBool32 xrfwEndFrame(XrTime predictedDisplayTime,
                               XrCompositionLayerProjection *layer) {
  if (!layer) {
    // no renderring
    XrFrameEndInfo frameEndInfo{
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime =
            predictedDisplayTime, //  frameState.predictedDisplayTime,
        .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        .layerCount = 0,
        .layers = nullptr,
    };
    auto result = xrEndFrame(g_session, &frameEndInfo);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrEndFrame: " << result;
      return false;
    }
    return true;
  } else {
    layer->space = g_currentSpace;
    const XrCompositionLayerBaseHeader *layers[] = {
        (const XrCompositionLayerBaseHeader *)layer};
    XrFrameEndInfo frameEndInfo{
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime =
            predictedDisplayTime, //  frameState.predictedDisplayTime,
        .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        .layerCount = 1,
        .layers = layers,
    };
    auto result = xrEndFrame(g_session, &frameEndInfo);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrEndFrame: " << result;
      return false;
    }
    return true;
  }
}
