#ifdef XR_USE_PLATFORM_WIN32
#endif

#include <plog/Log.h>

#include "xr_linear.h"
#include "xrfw_initialization.h"
#include <algorithm>
#include <list>
#include <openxr/openxr.h>

#include <unordered_map>
#include <vector>
#include <xrfw.h>

XrfwInitialization g_init = {};

static const int MAX_NUM_LAYERS = 16;
// App only supports the primary stereo view config.
const XrViewConfigurationType g_supportedViewConfigType =
  XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

XrInstance g_instance = nullptr;
XrSystemId g_systemId = {};
XrSession g_session = nullptr;
XrEventDataBuffer g_eventDataBuffer = {};
XrSessionState g_sessionState = XR_SESSION_STATE_UNKNOWN;
bool g_sessionRunning = false;
XrSpace g_currentSpace = {};
XrSpace g_headSpace = {};
XrSpace g_localSpace = {};

struct SwapchainInfo
{
  uint32_t index = 0;
  int width = 0;
  int height = 0;
  std::vector<XrSwapchainImageBaseHeader*> images;
  bool useVprt = false;
};
std::unordered_map<XrSwapchain, SwapchainInfo> g_swapchainImages;

XrFrameState g_frameState{ XR_TYPE_FRAME_STATE };
XrCompositionLayerProjectionView g_projectionViews[2];
XrCompositionLayerProjection g_projection = {};

static std::vector<int64_t>
_xrfwGetSwapchainFormats(XrSession session)
{
  // Select a swapchain format.
  uint32_t swapchainFormatCount;
  auto result =
    xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, nullptr);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrEnumerateSwapchainFormats");
  }

  // std::vector<int64_t> runtimeFormats;
  std::vector<int64_t> swapchainFormats(swapchainFormatCount);
  result = xrEnumerateSwapchainFormats(session,
                                       (uint32_t)swapchainFormats.size(),
                                       &swapchainFormatCount,
                                       swapchainFormats.data());
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrEnumerateSwapchainFormats");
  }
  assert(swapchainFormatCount == swapchainFormats.size());
  return swapchainFormats;
}

XRFW_API XrInstance
xrfwGetInstance()
{
  return g_instance;
}

// static std::vector<XrExtensionProperties>
// GetXrExtensionProperties()
// {
//   PFN_xrEnumerateInstanceExtensionProperties
//     xrEnumerateInstanceExtensionProperties;
//   auto result = xrGetInstanceProcAddr(
//     XR_NULL_HANDLE,
//     "xrEnumerateInstanceExtensionProperties",
//     (PFN_xrVoidFunction*)&xrEnumerateInstanceExtensionProperties);
//   if (result != XR_SUCCESS) {
//     PLOG_FATAL << "Failed to get xrEnumerateInstanceExtensionProperties";
//     return {};
//   }
//
//   uint32_t numInputExtensions = 0;
//   uint32_t numOutputExtensions = 0;
//   result = xrEnumerateInstanceExtensionProperties(
//     NULL, numInputExtensions, &numOutputExtensions, NULL);
//   if (result != XR_SUCCESS) {
//     PLOG_FATAL << "Failed to xrEnumerateInstanceExtensionProperties";
//     return {};
//   }
//   PLOG_INFO << "xrEnumerateInstanceExtensionProperties found "
//             << numOutputExtensions << " extension(s).";
//
//   numInputExtensions = numOutputExtensions;
//   std::vector<XrExtensionProperties> extensionProperties(
//     numOutputExtensions, { XR_TYPE_EXTENSION_PROPERTIES });
//
//   result = xrEnumerateInstanceExtensionProperties(
//     NULL, numInputExtensions, &numOutputExtensions,
//     extensionProperties.data());
//   if (result != XR_SUCCESS) {
//     PLOG_FATAL << "Failed to xrEnumerateInstanceExtensionProperties";
//     return {};
//   }
//   return extensionProperties;
// }

XRFW_API XrInstance
xrfwCreateInstance(const char* const* extensionNames,
                   uint32_t extensionCount,
                   XrApplicationInfo* pAppInfo,
                   XrSystemId* pOutSystemId)
{
  for (uint32_t i = 0; i < extensionCount; ++i) {
    g_init.extensionNames.push_back(extensionNames[i]);
  }
  xrfwInitLogger();

  XrApplicationInfo appInfo{
    .applicationName = "xrfw_app",
    .applicationVersion = 0,
    .engineName = "xrfw_engine",
    .engineVersion = 0,
    .apiVersion = XR_CURRENT_API_VERSION,
  };
  if (pAppInfo) {
    appInfo = *pAppInfo;
  }

  XrInstanceCreateInfo instanceCreateInfo{
    .type = XR_TYPE_INSTANCE_CREATE_INFO,
    .next = g_init.next,
    .createFlags = 0,
    .applicationInfo = appInfo,
    .enabledApiLayerCount = 0,
    .enabledApiLayerNames = nullptr,
    .enabledExtensionCount =
      static_cast<uint32_t>(g_init.extensionNames.size()),
    .enabledExtensionNames = g_init.extensionNames.data(),
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

  if (!g_init.graphicsRequirementsCallback(
        g_instance, g_systemId, g_init.graphicsRequirements)) {
    return nullptr;
  }

  if (pOutSystemId) {
    *pOutSystemId = g_systemId;
  }

  return g_instance;
}

XRFW_API void
xrfwDestroyInstance()
{
  xrDestroyInstance(g_instance);
}

XRFW_API XrSession
xrfwCreateSession(XrfwSwapchains* swapchains, const void* next, bool useVrpt)
{
  XrSessionCreateInfo sessionCreateInfo = {
    .type = XR_TYPE_SESSION_CREATE_INFO,
    .next = next, // &graphicsBindingGL,
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
    auto result = xrEnumerateViewConfigurations(
      g_instance, g_systemId, 0, &viewportConfigTypeCount, NULL);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrEnumerateViewConfigurations: " << result;
      return nullptr;
    }

    std::vector<XrViewConfigurationType> viewportConfigurationTypes(
      viewportConfigTypeCount);
    result = xrEnumerateViewConfigurations(g_instance,
                                           g_systemId,
                                           viewportConfigTypeCount,
                                           &viewportConfigTypeCount,
                                           viewportConfigurationTypes.data());
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

  // swapchain
  XrViewConfigurationView viewConfigurationViews[2];
  if (!xrfwGetViewConfigurationViews(viewConfigurationViews, 2)) {
    return {};
  }

  if (useVrpt) {
    swapchains->leftOrVrpt = xrfwCreateSwapchain(viewConfigurationViews[0],
                                                 &swapchains->format,
                                                 &swapchains->width,
                                                 &swapchains->height,
                                                 2);
    if (!swapchains->leftOrVrpt) {
      return {};
    }
  } else {
    swapchains->leftOrVrpt = xrfwCreateSwapchain(viewConfigurationViews[0],
                                                 &swapchains->format,
                                                 &swapchains->width,
                                                 &swapchains->height,
                                                 1);
    if (!swapchains->leftOrVrpt) {
      return {};
    }
    swapchains->right = xrfwCreateSwapchain(viewConfigurationViews[1],
                                            &swapchains->format,
                                            &swapchains->width,
                                            &swapchains->height,
                                            1);
    if (!swapchains->right) {
      return {};
    }
  }

  return g_session;
}

XRFW_API void
xrfwDestroySession(void* session)
{
  assert(session == g_session);
  xrDestroySession((XrSession)session);
}

XRFW_API XrSpace
xrfwAppSpace()
{
  return g_currentSpace;
}

XRFW_API XrBool32
xrfwGetViewConfigurationViews(XrViewConfigurationView* viewConfigurationViews,
                              uint32_t viewCount)
{

  for (int i = 0; i < viewCount; ++i) {
    viewConfigurationViews[i] = { XR_TYPE_VIEW_CONFIGURATION_VIEW };
  }
  auto result = xrEnumerateViewConfigurationViews(g_instance,
                                                  g_systemId,
                                                  g_supportedViewConfigType,
                                                  viewCount,
                                                  &viewCount,
                                                  viewConfigurationViews);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrEnumerateViewConfigurationViews: " << result;
    return {};
  }

  // Log the view config info for each view type for debugging purposes.
  for (uint32_t e = 0; e < viewCount; e++) {
    const XrViewConfigurationView* element = &viewConfigurationViews[e];

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

XRFW_API XrSwapchain
xrfwCreateSwapchain(const XrViewConfigurationView& viewConfigurationView,
                    uint64_t* format,
                    int* width,
                    int* height,
                    uint32_t arraySize)
{
  auto swapchainFormats = _xrfwGetSwapchainFormats(g_session);
  auto colorSwapchainFormat =
    g_init.selectColorSwapchainFormatCallback(swapchainFormats);
  *format = colorSwapchainFormat;

  PLOG_INFO << "Creating swapchain for view "
            << "with dimensions Width="
            << viewConfigurationView.recommendedImageRectWidth
            << " Height=" << viewConfigurationView.recommendedImageRectHeight
            << " Format=" << colorSwapchainFormat << " SampleCount="
            << viewConfigurationView.recommendedSwapchainSampleCount;

  // Create the swapchain.
  XrSwapchainCreateInfo swapchainCreateInfo{
    .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
    .next = nullptr,
    .createFlags = 0,
    .usageFlags =
      XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
    .format = colorSwapchainFormat,
    .sampleCount = viewConfigurationView.recommendedSwapchainSampleCount,
    .width = viewConfigurationView.recommendedImageRectWidth,
    .height = viewConfigurationView.recommendedImageRectHeight,
    .faceCount = 1,
    .arraySize = arraySize,
    .mipCount = 1,
  };

  XrSwapchain swapchain;
  auto result = xrCreateSwapchain(g_session, &swapchainCreateInfo, &swapchain);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return {};
  }

  uint32_t imageCount;
  result = xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return {};
  }

  // XXX This should really just return XrSwapchainImageBaseHeader*
  auto swapchainImages = g_init.allocateSwapchainImageStructsCallback(
    imageCount, swapchainCreateInfo);
  result = xrEnumerateSwapchainImages(
    swapchain, imageCount, &imageCount, swapchainImages[0]);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return {};
  }

  auto index = (uint32_t)g_swapchainImages.size();
  g_swapchainImages.insert(
    std::make_pair(swapchain,
                   SwapchainInfo{
                     .index = index,
                     .width = static_cast<int>(swapchainCreateInfo.width),
                     .height = static_cast<int>(swapchainCreateInfo.height),
                     .images = swapchainImages,
                     .useVprt = (arraySize == 2),
                   }));

  *width = swapchainCreateInfo.width;
  *height = swapchainCreateInfo.height;
  return swapchain;
}

XRFW_API const XrSwapchainImageBaseHeader*
xrfwAcquireSwapchain(XrSwapchain swapchain)
{
  auto found = g_swapchainImages.find(swapchain);
  if (found == g_swapchainImages.end()) {
    return {};
  }
  auto& info = found->second;
  g_projectionViews[info.index].subImage = {
      .swapchain = swapchain,
      .imageRect =
          {
              .offset = {0, 0},
              .extent = {info.width, info.height},
          },
      .imageArrayIndex = 0,
  };
  if (info.useVprt) {
    assert(info.index == 0);
    g_projectionViews[1].subImage = {
        .swapchain = swapchain,
        .imageRect =
            {
                .offset = {0, 0},
                .extent = {info.width, info.height},
            },
        .imageArrayIndex = 1,
    };
  }

  XrSwapchainImageAcquireInfo acquireInfo{
    XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO
  };
  uint32_t swapchainImageIndex;
  auto result =
    xrAcquireSwapchainImage(swapchain, &acquireInfo, &swapchainImageIndex);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrAcquireSwapchainImage: " << result;
    return {};
  }

  XrSwapchainImageWaitInfo waitInfo{
    .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
    .timeout = XR_INFINITE_DURATION,
  };
  result = xrWaitSwapchainImage(swapchain, &waitInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrWaitSwapchainImage: " << result;
    return {};
  }

  return g_swapchainImages[swapchain].images[swapchainImageIndex];
}

XRFW_API void
xrfwReleaseSwapchain(XrSwapchain swapchain)
{
  XrSwapchainImageReleaseInfo releaseInfo{
    XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO
  };
  auto result = xrReleaseSwapchainImage(swapchain, &releaseInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrReleaseSwapchainImage: " << result;
  }
}

// Return event if one is available, otherwise return null.
static const XrEventDataBaseHeader*
TryReadNextEvent()
{
  // It is sufficient to clear the just the XrEventDataBuffer header to
  // XR_TYPE_EVENT_DATA_BUFFER
  auto baseHeader =
    reinterpret_cast<XrEventDataBaseHeader*>(&g_eventDataBuffer);
  *baseHeader = { XR_TYPE_EVENT_DATA_BUFFER };
  auto result = xrPollEvent(g_instance, &g_eventDataBuffer);
  if (result == XR_SUCCESS) {
    if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
      const XrEventDataEventsLost* const eventsLost =
        reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
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

static void
HandleSessionStateChangedEvent(
  const XrEventDataSessionStateChanged& stateChangedEvent,
  SessionBeginFunc begin,
  SessionEndFunc end,
  void* user)
{
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
      if (begin) {
        begin(g_session, user);
      }
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
      if (end) {
        end(g_session, user);
      }
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

XRFW_API XrBool32
xrfwPollEventsIsSessionActive(SessionBeginFunc begin,
                              SessionEndFunc end,
                              void* user)
{
  // Process all pending messages.
  while (const XrEventDataBaseHeader* event = TryReadNextEvent()) {
    switch (event->type) {
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
        const auto& instanceLossPending =
          *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);
        PLOG_WARNING << "XrEventDataInstanceLossPending by "
                     << instanceLossPending.lossTime;
        break;
      }

      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
        auto sessionStateChangedEvent =
          *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
        HandleSessionStateChangedEvent(
          sessionStateChangedEvent, begin, end, user);
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

static void
poseToMatrix(XrMatrix4x4f* view, const XrPosef& pose)
{
  XrMatrix4x4f toView;
  XrVector3f scale{ 1.f, 1.f, 1.f };
  XrMatrix4x4f_CreateTranslationRotationScale(
    &toView, &pose.position, &pose.orientation, &scale);
  XrMatrix4x4f_InvertRigidBody(view, &toView);
}

XRFW_API XrCompositionLayerBaseHeader*
xrfwBeginFrame(XrTime* outtime, XrfwViewMatrices* viewMatrix)
{
  XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
  XrFrameState frameState{ XR_TYPE_FRAME_STATE };
  auto result = xrWaitFrame(g_session, &frameWaitInfo, &frameState);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return nullptr;
  }
  g_frameState = frameState;
  *outtime = frameState.predictedDisplayTime;

  static XrBool32 shouldRender_ = false;
  if (shouldRender_ != frameState.shouldRender) {
    PLOG_INFO << "shouldRender: " << shouldRender_ << " => "
              << frameState.shouldRender;
    shouldRender_ = frameState.shouldRender;
  }

  XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
  result = xrBeginFrame(g_session, &frameBeginInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return nullptr;
  }

  XrView views[2]{
    { XR_TYPE_VIEW },
    { XR_TYPE_VIEW },
  };
  if (shouldRender_) {
    // view
    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    XrViewLocateInfo viewLocateInfo{
      .type = XR_TYPE_VIEW_LOCATE_INFO,
      .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
      .displayTime = frameState.predictedDisplayTime,
      .space = g_currentSpace,
    };
    uint32_t viewCountOutput;
    auto result = xrLocateViews(
      g_session, &viewLocateInfo, &viewState, 2, &viewCountOutput, views);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrLocateViews: " << result;
      return nullptr;
    }
    if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
      return nullptr; // There is no valid tracking poses for the views.
    }
    assert(viewCountOutput == 2);

    // update matrix
    XrMatrix4x4f_CreateProjectionFov((XrMatrix4x4f*)viewMatrix->leftProjection,
                                     GRAPHICS_OPENGL,
                                     views[0].fov,
                                     0.05f,
                                     100.0f);
    poseToMatrix((XrMatrix4x4f*)viewMatrix->leftView, views[0].pose);
    XrMatrix4x4f_CreateProjectionFov((XrMatrix4x4f*)viewMatrix->rightProjection,
                                     GRAPHICS_OPENGL,
                                     views[1].fov,
                                     0.05f,
                                     100.0f);
    poseToMatrix((XrMatrix4x4f*)viewMatrix->rightView, views[1].pose);
  }

  g_projectionViews[0].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
  g_projectionViews[0].pose = views[0].pose;
  g_projectionViews[0].fov = views[0].fov;

  g_projectionViews[1].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
  g_projectionViews[1].pose = views[1].pose;
  g_projectionViews[1].fov = views[1].fov;

  g_projection = {
    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
    .next = nullptr,
    .layerFlags = 0,
    .space = g_currentSpace,
    .viewCount = 2,
    .views = g_projectionViews,
  };

  return shouldRender_ ? (XrCompositionLayerBaseHeader*)&g_projection : nullptr;
}

XRFW_API XrBool32
xrfwEndFrame(const XrCompositionLayerBaseHeader* const* layers,
             uint32_t layerCount)
{
  XrFrameEndInfo frameEndInfo = {
    .type = XR_TYPE_FRAME_END_INFO,
    .displayTime = g_frameState.predictedDisplayTime,
    .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
    .layerCount = layerCount,
    .layers = layers,
  };

  auto result = xrEndFrame(g_session, &frameEndInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrEndFrame: " << result;
    return false;
  }
  return true;
}
