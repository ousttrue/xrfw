#include "desktop_capture.h"
#include <d3d11_4.h>
#include <grapho/dx11/texture.h>
#include <inspectable.h>
#include <windows.graphics.capture.h>
#include <windows.graphics.capture.interop.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/base.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.graphics.capture.h>
#include <winrt/windows.graphics.directx.direct3d11.h>

#include <ScreenCaptureforHWND/direct3d11.interop.h>

struct rgba
{
  uint8_t x, y, z, w;
};
static rgba pixels[4] = {
  { 255, 0, 0, 255 },
  { 0, 255, 0, 255 },
  { 0, 0, 255, 255 },
  { 255, 255, 255, 255 },
};

struct DesktopCaptureImpl
{
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device_rt;
  winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_capture_item{
    nullptr
  };
  winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_frame_pool{
    nullptr
  };
  winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::
    FrameArrived_revoker m_frame_arrived;
  winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_capture_session{
    nullptr
  };

  DesktopCaptureImpl()
  {
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    // If the project is in a debug build, enable the debug layer.
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // デバイス作成
    ::D3D11CreateDevice(nullptr,
                        D3D_DRIVER_TYPE_HARDWARE,
                        nullptr,
                        creationFlags,
                        nullptr,
                        0,
                        D3D11_SDK_VERSION,
                        m_device.put(),
                        nullptr,
                        nullptr);

    // WinRT 版デバイス作成
    auto dxgi = m_device.as<IDXGIDevice>();
    winrt::com_ptr<::IInspectable> device_rt;
    ::CreateDirect3D11DeviceFromDXGIDevice(dxgi.get(), device_rt.put());
    m_device_rt =
      device_rt
        .as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
  }

  bool Start(HMONITOR monitor)
  {
    // CaptureItem 作成
    auto factory = winrt::get_activation_factory<
      winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop = factory.as<IGraphicsCaptureItemInterop>();

    interop->CreateForMonitor(
      monitor,
      winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
      winrt::put_abi(m_capture_item));
    if (m_capture_item) {
      // FramePool 作成
      auto size = m_capture_item.Size();
      m_frame_pool =
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::
          CreateFreeThreaded(m_device_rt,
                             winrt::Windows::Graphics::DirectX::
                               DirectXPixelFormat::B8G8R8A8UIntNormalized,
                             1,
                             size);
      m_frame_arrived = m_frame_pool.FrameArrived(
        winrt::auto_revoke, { this, &DesktopCaptureImpl::onFrameArrived });

      // キャプチャ開始
      m_capture_session = m_frame_pool.CreateCaptureSession(m_capture_item);
      m_capture_session.StartCapture();
      return true;
    } else {
      return false;
    }
  }

  void Stop()
  {
    m_frame_arrived.revoke();
    m_capture_session = nullptr;
    if (m_frame_pool) {
      m_frame_pool.Close();
      m_frame_pool = nullptr;
    }
    m_capture_item = nullptr;
    // m_callback = {};
  }

  void onFrameArrived(
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
    winrt::Windows::Foundation::IInspectable const& args)
  {
    auto frame = sender.TryGetNextFrame();
    auto size =
      frame
        .ContentSize(); // ウィンドウのサイズ。テキスチャのサイズと一致するとは限らない

    winrt::com_ptr<ID3D11Texture2D> surface; // これがキャプチャ結果
    frame.Surface().as<IDirect3DDxgiInterfaceAccess>()->GetInterface(
      winrt::guid_of<ID3D11Texture2D>(), surface.put_void());

    // m_callback(surface.get(), size.Width, size.Height);
  }

  std::shared_ptr<grapho::dx11::Texture> m_texture;

  HANDLE CreateShared()
  {
    m_texture =
      grapho::dx11::Texture::Create(m_device, 2, 2, &pixels[0].x, true);
    auto resource = m_texture->Texture2D.as<IDXGIResource>();
    if (!resource) {
      return nullptr;
    }
    HANDLE handle;
    auto hr = resource->GetSharedHandle(&handle);
    if (FAILED(hr)) {
      return nullptr;
    }

    winrt::com_ptr<ID3D11DeviceContext> context;
    m_device->GetImmediateContext(context.put());
    context->Flush();

    return handle;
  }
};

DesktopCapture::DesktopCapture()
  : m_impl(new DesktopCaptureImpl)
{
}

DesktopCapture::~DesktopCapture()
{
  delete m_impl;
}

HANDLE
DesktopCapture::CreateShared()
{
  return m_impl->CreateShared();
}
