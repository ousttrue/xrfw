#pragma once
#include <openxr/openxr.h>

struct XrfwSwapchains
{
  XrSwapchain leftOrVrpt = nullptr;
  XrSwapchain right = nullptr;
  uint64_t format = 0;
  int width = 0;
  int height = 0;
};

using RenderFunc =
  void (*)(XrTime time,
           const XrSwapchainImageBaseHeader* leftOrVrptSwapchainImage,
           const XrSwapchainImageBaseHeader* rightSwapchainImage,
           const XrfwSwapchains& info,
           const float projection[16],
           const float view[16],
           const float rightProjection[16],
           const float rightView[16],
           void* user);

using SessionBeginFunc = void (*)(XrSession session, void* user);

using SessionEndFunc = void (*)(XrSession session, void* user);
