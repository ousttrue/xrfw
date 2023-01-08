#pragma once
#include <openxr/openxr.h>
#include <optional>
#include <vector>

class OxrRenderer {
  XrInstance instance_ = nullptr;
  XrSession session_ = nullptr;

  XrSpace currentSpace_ = {};
  XrSpace headSpace_ = {};
  XrSpace localSpace_ = {};

  std::vector<XrCompositionLayerBaseHeader *> layers;

  struct Swapchain {
    XrSwapchain handle;
    int32_t width;
    int32_t height;
  };
  Swapchain swapchains_[2];

public:
  OxrRenderer(XrInstance instance, XrSession session);
  ~OxrRenderer();
  bool Initialize();
  // waitFrame, beginFrame, render and endFrame
  void RenderFrame();

private:
  std::optional<XrCompositionLayerProjection>
  RenderLayer(XrTime predictedDisplayTime);
};
