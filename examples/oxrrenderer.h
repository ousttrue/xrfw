#pragma once
#include <openxr/openxr.h>
#include <optional>

class OxrRenderer {
  XrInstance instance_ = nullptr;
  XrSession session_ = nullptr;

public:
  OxrRenderer(XrInstance instance, XrSession session);
  ~OxrRenderer();
  std::optional<XrCompositionLayerProjectionView>
  RenderLayer(XrSwapchain swapchain, int width, int height,
              XrTime predictedDisplayTime, const XrView &view);
};
