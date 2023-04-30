#pragma once
#include <d3d11.h>

class DesktopCapture
{
  struct DesktopCaptureImpl *m_impl;
public:
  DesktopCapture();
  ~DesktopCapture();
  HANDLE CreateShared();
};

