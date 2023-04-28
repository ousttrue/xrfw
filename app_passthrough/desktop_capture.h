#pragma once

class DesktopCapture
{
  struct DesktopCaptureImpl *m_impl;
public:
  DesktopCapture();
  ~DesktopCapture();
};

