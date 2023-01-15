#ifdef XR_USE_PLATFORM_WIN32
#ifdef XR_USE_GRAPHICS_API_D3D11
#include <xrfw_impl_win32_d3d11.h>
#include <algorithm>
#include <plog/Log.h>
#include <vector>
#include <winrt/base.h> // winrt::com_ptr
#include <xrfw.h>

#include "DxUtility.h"

struct PlatformWin32D3D11Impl {

  XrGraphicsRequirementsD3D11KHR graphicsRequirements_;
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;

  static void CreateD3D11DeviceAndContext(
      IDXGIAdapter1 *adapter,
      const std::vector<D3D_FEATURE_LEVEL> &featureLevels,
      ID3D11Device **device, ID3D11DeviceContext **deviceContext) {
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Create the Direct3D 11 API device object and a corresponding context.
    D3D_DRIVER_TYPE driverType =
        adapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN;

  TryAgain:
    const HRESULT hr =
        D3D11CreateDevice(adapter, driverType, 0, creationFlags,
                          featureLevels.data(), (UINT)featureLevels.size(),
                          D3D11_SDK_VERSION, device, nullptr, deviceContext);

    if (FAILED(hr)) {
      // If initialization failed, it may be because device debugging isn't
      // supprted, so retry without that.
      if ((creationFlags & D3D11_CREATE_DEVICE_DEBUG) &&
          (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)) {
        creationFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
        goto TryAgain;
      }

      // If the initialization still fails, fall back to the WARP device.
      // For more information on WARP, see:
      // http://go.microsoft.com/fwlink/?LinkId=286690
      if (driverType != D3D_DRIVER_TYPE_WARP) {
        driverType = D3D_DRIVER_TYPE_WARP;
        goto TryAgain;
      }
    }
  }

  winrt::com_ptr<ID3D11Device> InitializeGraphics() {
    // Create a list of feature levels which are both supported by the OpenXR
    // runtime and this application.
    std::vector<D3D_FEATURE_LEVEL> featureLevels = {
        D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    featureLevels.erase(
        std::remove_if(featureLevels.begin(), featureLevels.end(),
                       [&](D3D_FEATURE_LEVEL fl) {
                         return fl < graphicsRequirements_.minFeatureLevel;
                       }),
        featureLevels.end());
    if (featureLevels.size() == 0) {
      PLOG_FATAL << "Unsupported minimum feature level!";
      return {};
    }

    const winrt::com_ptr<IDXGIAdapter1> adapter =
        sample::dx::GetAdapter(graphicsRequirements_.adapterLuid);
    CreateD3D11DeviceAndContext(adapter.get(), featureLevels, m_device.put(),
                                m_deviceContext.put());
    return m_device;
  }

  XrSession CreateSession(XrfwSwapchains *swapchains) {
    return xrfwCreateSessionWin32D3D11(swapchains, m_device.get());
  }
};

XrfwPlatformWin32D3D11::XrfwPlatformWin32D3D11(struct android_app *)
    : impl_(new PlatformWin32D3D11Impl) {}
XrfwPlatformWin32D3D11::~XrfwPlatformWin32D3D11() { delete impl_; }
XrInstance XrfwPlatformWin32D3D11::CreateInstance() {
  xrfwInitExtensionsWin32D3D11(&impl_->graphicsRequirements_);
  return xrfwCreateInstance();
}
winrt::com_ptr<ID3D11Device> XrfwPlatformWin32D3D11::InitializeGraphics() {
  return impl_->InitializeGraphics();
}
XrSession XrfwPlatformWin32D3D11::CreateSession(XrfwSwapchains *swapchains) {
  return impl_->CreateSession(swapchains);
}
bool XrfwPlatformWin32D3D11::BeginFrame() { return true; }
void XrfwPlatformWin32D3D11::EndFrame(RenderFunc render, void *user) {}
void XrfwPlatformWin32D3D11::Sleep(std::chrono::milliseconds ms) { {}; }
#endif
#endif