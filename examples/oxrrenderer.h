#pragma once
#include <openxr/openxr.h>
#include <optional>

class OxrRenderer {
  XrInstance instance_ = nullptr;
  XrSession session_ = nullptr;

  struct Swapchain {
    XrSwapchain handle;
    int32_t width;
    int32_t height;
  };
  Swapchain swapchains_[2];

  XrCompositionLayerProjectionView projectionLayerViews_[2];

public:
  OxrRenderer(XrInstance instance, XrSession session);
  ~OxrRenderer();
  bool CreateSwapchains(int viewCount);
  std::optional<XrCompositionLayerProjection>
  RenderLayer(XrTime predictedDisplayTime, const XrView views[2]);
};
