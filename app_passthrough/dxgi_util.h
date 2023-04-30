#pragma once
#include <d3d11.h>
#include <plog/Log.h>
#include <winrt/base.h>

inline void
EnumOutput(const winrt::com_ptr<ID3D11Device>& device)
{
  auto obj = device.as<IDXGIObject>();
  winrt::com_ptr<IDXGIAdapter1> adapter;
  obj->GetParent(IID_PPV_ARGS(adapter.put()));
  DXGI_ADAPTER_DESC adapter_desc;
  adapter->GetDesc(&adapter_desc);
  PLOG_INFO << "ADAPTER: " << adapter_desc.Description;
  winrt::com_ptr<IDXGIOutput> output;
  for (int i = 0; adapter->EnumOutputs(i, output.put()) != DXGI_ERROR_NOT_FOUND;
       ++i, output.detach()) {
    DXGI_OUTPUT_DESC output_desc;
    output->GetDesc(&output_desc);
    PLOG_INFO << "output[" << i << "] " << output_desc.DeviceName << " "
              << (output_desc.DesktopCoordinates.right -
                  output_desc.DesktopCoordinates.left)
              << ": "
              << (output_desc.DesktopCoordinates.bottom -
                  output_desc.DesktopCoordinates.top);
    ;
  }
}
