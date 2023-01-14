#pragma once
#include <chrono>
#include <openxr/openxr.h>
#include <ostream>
#include <span>
#include <stdint.h>
#include <string_view>
#include <vector>

struct XrfwSwapchains {
  XrSwapchain left;
  int leftWidth;
  int leftHeight;
  XrSwapchain right;
  int rightWidth;
  int rightHeight;
};

#ifdef XR_USE_PLATFORM_WIN32
#ifdef XRFW_BUILD
#define XRFW_API extern "C" __declspec(dllexport)
#else
#define XRFW_API extern "C" __declspec(dllimport)
#endif
#else
#define XRFW_API
#endif

XRFW_API void xrfwInitLogger();
XRFW_API XrInstance xrfwCreateInstance(XrApplicationInfo *appInfo = nullptr);
XRFW_API void xrfwDestroyInstance();
XRFW_API XrInstance xrfwGetInstance();

XRFW_API XrSession xrfwCreateSession(XrfwSwapchains *swapchains,
                                     const void *next);
XRFW_API void xrfwDestroySession(void *session);
XRFW_API XrSpace xrfwAppSpace();

XRFW_API XrBool32 xrfwGetViewConfigurationViews(
    XrViewConfigurationView *viewConfigurationViews, uint32_t viewCount);
XRFW_API XrSwapchain
xrfwCreateSwapchain(const XrViewConfigurationView &viewConfigurationView,
                    int *width, int *height);
XRFW_API const XrSwapchainImageBaseHeader *
xrfwAcquireSwapchain(XrSwapchain swapchain);
XRFW_API void xrfwReleaseSwapchain(XrSwapchain swapchain);

XRFW_API XrBool32 xrfwPollEventsIsSessionActive();

struct XrfwViewMatrices {
  float leftProjection[16];
  float leftView[16];
  float rightProjection[16];
  float rightView[16];
};
XRFW_API XrBool32 xrfwBeginFrame(XrTime *outtime, XrfwViewMatrices *viewMatrix);
XRFW_API XrBool32 xrfwEndFrame();

using RenderFunc = void (*)(const XrSwapchainImageBaseHeader *swapchainImage,
                            int width, int height, const float projection[16],
                            const float view[16], void *user);

#ifdef XR_USE_PLATFORM_WIN32
#include "xrfw_win32.h"
#elif XR_USE_PLATFORM_ANDROID
#include "xrfw_android.h"
#else
#error "XR_USE_PLATFORM required"
#endif

//

template <typename T>
inline int xrfwSession(T &platform, const RenderFunc &render, void *user) {
  // session and swapchains from graphics
  XrfwSwapchains swapchains;
  auto session = platform.CreateSession(&swapchains);
  if (!session) {
    return 3;
  }

  // glfw mainloop
  while (platform.BeginFrame()) {

    // OpenXR handling
    if (xrfwPollEventsIsSessionActive()) {
      XrTime frameTime;
      XrfwViewMatrices viewMatrix;
      if (xrfwBeginFrame(&frameTime, &viewMatrix)) {
        // left
        if (auto swapchainImage = xrfwAcquireSwapchain(swapchains.left)) {
          render(swapchainImage, swapchains.leftWidth, swapchains.leftHeight,
                 viewMatrix.leftProjection, viewMatrix.leftView, user);
          xrfwReleaseSwapchain(swapchains.left);
        }
        // right
        if (auto swapchainImage = xrfwAcquireSwapchain(swapchains.right)) {
          render(swapchainImage, swapchains.rightWidth, swapchains.rightHeight,
                 viewMatrix.rightProjection, viewMatrix.rightView, user);
          xrfwReleaseSwapchain(swapchains.right);
        }
      }
      xrfwEndFrame();
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
inline std::ostream &operator<<(std::ostream &os, XrResult value) {
  char buffer[XR_MAX_RESULT_STRING_SIZE];
  if (XR_SUCCEEDED(xrResultToString(xrfwGetInstance(), value, buffer))) {
    os << std::string_view(buffer);
  } else {
    os << value;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os, XrStructureType value) {
  char buffer[XR_MAX_STRUCTURE_NAME_SIZE];
  if (XR_SUCCEEDED(xrStructureTypeToString(xrfwGetInstance(), value, buffer))) {
    os << std::string_view(buffer);
  } else {
    os << value;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os, XrSessionState value) {
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
