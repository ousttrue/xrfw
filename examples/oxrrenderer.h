#pragma once
#include <openxr/openxr.h>
#include <vector>

class OxrRenderer {
  XrInstance instance_ = nullptr;
  XrSession session_ = nullptr;

  XrSpace currentSpace_ = {};
  XrSpace headSpace_ = {};
  XrSpace localSpace_ = {};

  XrView projections[2]{
      {XR_TYPE_VIEW},
      {XR_TYPE_VIEW},
  };

  std::vector<XrCompositionLayerBaseHeader *> layers;

public:
  OxrRenderer(XrInstance instance, XrSession session);
  ~OxrRenderer();
  bool Initialize();
  // waitFrame, beginFrame, render and endFrame
  void RenderFrame();
};
