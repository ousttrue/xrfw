#pragma once
#include <openxr/openxr.h>

class OxrApp {
  XrInstance instance_ = nullptr;
  XrSession session_ = nullptr;

  XrSpace currentSpace_ = {};
  XrSpace headSpace_ = {};
  XrSpace localSpace_ = {};

  XrView projections[2]{
      {XR_TYPE_VIEW},
      {XR_TYPE_VIEW},
  };

public:
  OxrApp(XrInstance instance, XrSession session);
  ~OxrApp();
  bool Initialize();
  void ProcessFrame();

private:
  void RenderFrame();
};
