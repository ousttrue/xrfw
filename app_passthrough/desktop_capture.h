#pragma once
#include <d3d11.h>
#include <winrt/base.h>

class DesktopCapture
{
  struct DesktopCaptureImpl* m_impl;

public:
  DesktopCapture();
  ~DesktopCapture();
  HANDLE Start(const winrt::com_ptr<IDXGIOutput>& output);
};
