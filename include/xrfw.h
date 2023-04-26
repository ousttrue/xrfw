#pragma once
#include "xrfw_func.h"
#include <chrono>
#include <ostream>
#include <span>
#include <stdint.h>
#include <string_view>
#include <vector>

#ifdef XR_USE_PLATFORM_WIN32
#ifdef XRFW_BUILD
#define XRFW_API extern "C" __declspec(dllexport)
#else
#define XRFW_API extern "C" __declspec(dllimport)
#endif
#else
#define XRFW_API
#endif

XRFW_API void
xrfwInitLogger();
XRFW_API XrInstance
xrfwCreateInstance(const char* const* extensionNames = nullptr,
                   uint32_t extensionCount = 0,
                   XrApplicationInfo* appInfo = nullptr,
                   XrSystemId* pOutSystemId = nullptr);

XRFW_API void
xrfwDestroyInstance();
XRFW_API XrInstance
xrfwGetInstance();

XRFW_API XrSession
xrfwCreateSession(XrfwSwapchains* swapchains, const void* next, bool useVrpt);
XRFW_API void
xrfwDestroySession(void* session);
XRFW_API XrSpace
xrfwAppSpace();

XRFW_API XrBool32
xrfwGetViewConfigurationViews(XrViewConfigurationView* viewConfigurationViews,
                              uint32_t viewCount);
XRFW_API XrSwapchain
xrfwCreateSwapchain(const XrViewConfigurationView& viewConfigurationView,
                    uint64_t* format,
                    int* width,
                    int* height,
                    uint32_t arraySize);
XRFW_API const XrSwapchainImageBaseHeader*
xrfwAcquireSwapchain(XrSwapchain swapchain);
XRFW_API void
xrfwReleaseSwapchain(XrSwapchain swapchain);

XRFW_API XrBool32
xrfwPollEventsIsSessionActive(SessionBeginFunc begin,
                              SessionEndFunc end,
                              void* user);

struct XrfwViewMatrices
{
  float leftProjection[16];
  float leftView[16];
  float rightProjection[16];
  float rightView[16];
};
XRFW_API XrCompositionLayerBaseHeader*
xrfwBeginFrame(XrTime* outtime, XrfwViewMatrices* viewMatrix);
XRFW_API XrBool32
xrfwEndFrame(const XrCompositionLayerBaseHeader* const* layers,
             uint32_t layerCount);

#ifdef XR_USE_PLATFORM_WIN32
#include "xrfw_win32.h"
#elif XR_USE_PLATFORM_ANDROID
#include "xrfw_android.h"
#else
#error "XR_USE_PLATFORM required"
#endif

//

template<typename T>
inline int
xrfwSession(T& platform,
            RenderFunc render,
            void* user,
            SessionBeginFunc begin = nullptr,
            SessionEndFunc end = nullptr)
{
  // session and swapchains from graphics
  XrfwSwapchains swapchains = {};
  auto session = platform.CreateSession(&swapchains);
  if (!session) {
    return 3;
  }
  auto use_vrpt = swapchains.right == nullptr;

  // glfw mainloop
  while (platform.BeginFrame()) {

    // OpenXR handling
    if (xrfwPollEventsIsSessionActive(begin, end, user)) {
      XrTime frameTime;
      XrfwViewMatrices viewMatrix;
      if (auto projectionLayer = xrfwBeginFrame(&frameTime, &viewMatrix)) {
        if (use_vrpt) {
          if (auto swapchainImage =
                xrfwAcquireSwapchain(swapchains.leftOrVrpt)) {
            render(frameTime,
                   swapchainImage,
                   nullptr,
                   swapchains,
                   viewMatrix.leftProjection,
                   viewMatrix.leftView,
                   viewMatrix.rightProjection,
                   viewMatrix.rightView,
                   user);
            xrfwReleaseSwapchain(swapchains.leftOrVrpt);
          }
        } else {
          if (auto leftSwapchainImage =
                xrfwAcquireSwapchain(swapchains.leftOrVrpt)) {
            if (auto rightSwapchainImage =
                  xrfwAcquireSwapchain(swapchains.right)) {
              render(frameTime,
                     leftSwapchainImage,
                     rightSwapchainImage,
                     swapchains,
                     viewMatrix.leftProjection,
                     viewMatrix.leftView,
                     viewMatrix.rightProjection,
                     viewMatrix.rightView,
                     user);
              xrfwReleaseSwapchain(swapchains.right);
            }
            xrfwReleaseSwapchain(swapchains.leftOrVrpt);
          }
        }
        xrfwEndFrame(&projectionLayer, 1);
        //   const XrCompositionLayerBaseHeader* layers[] = { (
        //     const XrCompositionLayerBaseHeader*)&g_projection };
        //   frameEndInfo = {
        //     .type = XR_TYPE_FRAME_END_INFO,
        //     .displayTime = g_frameState.predictedDisplayTime,
        //     .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        //     .layerCount = layerCount,
        //     .layers = layers,
        //   };
      } else {
        xrfwEndFrame({}, {});
      }
    } else {
      // XrSession is not active
      platform.Sleep(std::chrono::milliseconds(30));
    }

    platform.EndFrame(render, user);
  }

  xrfwDestroySession(session);
  return 0;
}

// ostream
#include <iosfwd>
inline std::ostream&
operator<<(std::ostream& os, XrResult value)
{
  char buffer[XR_MAX_RESULT_STRING_SIZE];
  if (XR_SUCCEEDED(xrResultToString(xrfwGetInstance(), value, buffer))) {
    os << std::string_view(buffer);
  } else {
    os << value;
  }
  return os;
}

inline std::ostream&
operator<<(std::ostream& os, XrStructureType value)
{
  char buffer[XR_MAX_STRUCTURE_NAME_SIZE];
  if (XR_SUCCEEDED(xrStructureTypeToString(xrfwGetInstance(), value, buffer))) {
    os << std::string_view(buffer);
  } else {
    os << value;
  }
  return os;
}

inline std::ostream&
operator<<(std::ostream& os, XrSessionState value)
{
  switch (value) {
    case XR_SESSION_STATE_UNKNOWN:
      os << std::string_view("XR_SESSION_STATE_UNKNOWN");
      break;
    case XR_SESSION_STATE_IDLE:
      os << std::string_view("XR_SESSION_STATE_IDLE");
      break;
    case XR_SESSION_STATE_READY:
      os << std::string_view("XR_SESSION_STATE_READY");
      break;
    case XR_SESSION_STATE_SYNCHRONIZED:
      os << std::string_view("XR_SESSION_STATE_SYNCHRONIZED");
      break;
    case XR_SESSION_STATE_VISIBLE:
      os << std::string_view("XR_SESSION_STATE_VISIBLE");
      break;
    case XR_SESSION_STATE_FOCUSED:
      os << std::string_view("XR_SESSION_STATE_FOCUSED");
      break;
    case XR_SESSION_STATE_STOPPING:
      os << std::string_view("XR_SESSION_STATE_STOPPING");
      break;
    case XR_SESSION_STATE_LOSS_PENDING:
      os << std::string_view("XR_SESSION_STATE_LOSS_PENDING");
      break;
    case XR_SESSION_STATE_EXITING:
      os << std::string_view("XR_SESSION_STATE_EXITING");
      break;
    default:
      os << value;
      break;
  }
  return os;
}
