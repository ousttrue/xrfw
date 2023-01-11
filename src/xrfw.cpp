#ifdef XR_USE_PLATFORM_WIN32
#define PLOG_EXPORT
#endif

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

#include "xr_linear.h"
#include <algorithm>
#include <list>
#include <openxr/openxr.h>

#include <unordered_map>
#include <vector>
#include <xrfw.h>

static const int MAX_NUM_LAYERS = 16;

XrInstance g_instance = nullptr;
XrSystemId g_systemId = {};
XrSession g_session = nullptr;
XrEventDataBuffer g_eventDataBuffer = {};
XrSessionState g_sessionState = XR_SESSION_STATE_UNKNOWN;
bool g_sessionRunning = false;
XrSpace g_currentSpace = {};
XrSpace g_headSpace = {};
XrSpace g_localSpace = {};

XrFrameState g_frameState{XR_TYPE_FRAME_STATE};
XrCompositionLayerProjectionView g_projectionLayerViews[2] = {};
XrCompositionLayerProjection g_projection = {};

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

  if (!_xrfwGraphicsRequirements()) {
    return nullptr;
  }

  return g_instance;
}

XRFW_API void xrfwDestroyInstance() { xrDestroyInstance(g_instance); }

XRFW_API void xrfwDestroySession(void *session) {
  assert(session == g_session);
  xrDestroySession((XrSession)session);
}

XRFW_API XrSpace xrfwAppSpace() { return g_currentSpace; }

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

XRFW_API XrBool32 xrfwPollEventsIsSessionActive() {
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

static void poseToMatrix(XrMatrix4x4f *view, const XrPosef &pose) {
  XrMatrix4x4f toView;
  XrVector3f scale{1.f, 1.f, 1.f};
  XrMatrix4x4f_CreateTranslationRotationScale(&toView, &pose.position,
                                              &pose.orientation, &scale);
  XrMatrix4x4f_InvertRigidBody(view, &toView);
}

XRFW_API XrBool32 xrfwBeginFrame(XrTime *outtime,
                                 XrfwViewMatrices *viewMatrix) {
  XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState frameState{XR_TYPE_FRAME_STATE};
  auto result = xrWaitFrame(g_session, &frameWaitInfo, &frameState);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return false;
  }
  g_frameState = frameState;
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

  XrView views[2]{
      {XR_TYPE_VIEW},
      {XR_TYPE_VIEW},
  };
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

    // update matrix
    XrMatrix4x4f_CreateProjectionFov((XrMatrix4x4f *)viewMatrix->leftProjection,
                                     GRAPHICS_OPENGL, views[0].fov, 0.05f,
                                     100.0f);
    poseToMatrix((XrMatrix4x4f *)viewMatrix->leftView, views[0].pose);
    XrMatrix4x4f_CreateProjectionFov(
        (XrMatrix4x4f *)viewMatrix->rightProjection, GRAPHICS_OPENGL,
        views[1].fov, 0.05f, 100.0f);
    poseToMatrix((XrMatrix4x4f *)viewMatrix->rightView, views[1].pose);
  }

  g_projection = {
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
      .next = nullptr,
      .layerFlags = 0,
      .space = g_currentSpace,
      .viewCount = 2,
      .views = g_projectionLayerViews,
  };
  g_projectionLayerViews[0] = {
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
      .pose = views[0].pose,
      .fov = views[0].fov,
  };
  g_projectionLayerViews[1] = {
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
      .pose = views[1].pose,
      .fov = views[1].fov,
  };

  return shouldRender_;
}

XRFW_API XrBool32 xrfwEndFrame() {
  XrFrameEndInfo frameEndInfo = {};
  if (!g_frameState.shouldRender) {
    // no renderring
    frameEndInfo = {
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = g_frameState.predictedDisplayTime,
        .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        .layerCount = 0,
        .layers = nullptr,
    };
  } else {
    const XrCompositionLayerBaseHeader *layers[] = {
        (const XrCompositionLayerBaseHeader *)&g_projection};
    frameEndInfo = {
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = g_frameState.predictedDisplayTime,
        .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        .layerCount = 1,
        .layers = layers,
    };
  }

  auto result = xrEndFrame(g_session, &frameEndInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrEndFrame: " << result;
    return false;
  }
  return true;
}
