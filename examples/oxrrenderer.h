#pragma once
#include <openxr/openxr.h>
#include <vector>

class OxrRenderer {
  XrInstance instance_ = nullptr;
  XrSession session_ = nullptr;

  XrSpace currentSpace_ = {};
  XrSpace headSpace_ = {};
  XrSpace localSpace_ = {};

  std::vector<XrCompositionLayerBaseHeader *> layers;

public:
  OxrRenderer(XrInstance instance, XrSession session);
  ~OxrRenderer();
  bool Initialize();
  // waitFrame, beginFrame, render and endFrame
  void RenderFrame();

private:
  bool RenderLayer(XrTime predictedDisplayTime,
                   XrCompositionLayerProjection &layer);
};
