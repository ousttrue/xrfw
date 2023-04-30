#include "../app_ext_hand_tracking/xr_ext_hand_tracking.h"
#include "desktop_capture.h"
#include "xr_fb_paththrough.h"
#include <cuber/dx/DxCubeStereoRenderer.h>
#include <grapho/dx11/texture.h>
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>

struct rgba
{
  uint8_t x, y, z, w;
};

struct Context
{
  static const int MAX_NUM_EYES = 2;
  DesktopCapture m_capture;

  // d3d11
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;
  winrt::com_ptr<ID3D11Texture2D> m_depthStencil;
  winrt::com_ptr<ID3D11DepthStencilView> m_dsv;
  cuber::dx11::DxCubeStereoRenderer graphics;
  std::vector<cuber::Instance> m_instances;

  ExtHandTracking m_ext;
  std::shared_ptr<ExtHandTracker> m_trackerL;
  std::shared_ptr<ExtHandTracker> m_trackerR;
  FBPassthrough m_passthrough;
  std::shared_ptr<FBPassthroughFeature> m_passthroughFeature;

  std::shared_ptr<grapho::dx11::Texture> m_texture;

  Context(XrInstance instance,
          XrSystemId system,
          const winrt::com_ptr<ID3D11Device>& device)
    : m_device(device)
    , graphics(device)
    , m_ext(instance, system)
    , m_passthrough(instance, system)
  {
    m_device->GetImmediateContext(m_deviceContext.put());

    static rgba pixels[4] = {
      { 255, 0, 0, 255 },
      { 0, 255, 0, 255 },
      { 0, 0, 255, 255 },
      { 255, 255, 255, 255 },
    };
    m_texture = grapho::dx11::Texture::Create(device, 2, 2, &pixels[0].x);
  }

  bool CreateDepth(UINT width, UINT height)
  {
    if (m_dsv) {
      return true;
    }

    D3D11_TEXTURE2D_DESC depthDesc = {  
      .Width = width,
      .Height = height,
      .MipLevels = 1,
      .ArraySize = 2,
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .SampleDesc={
        .Count = 1,
        .Quality = 0,
      },
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_DEPTH_STENCIL,
      .CPUAccessFlags = 0,
      .MiscFlags = 0,
    };
    auto hr = m_device->CreateTexture2D(&depthDesc, NULL, m_depthStencil.put());
    if (FAILED(hr)) {
      return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {
      .Format = depthDesc.Format,
      .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
      .Texture2DArray = { 
        .MipSlice = 0, 
        .FirstArraySlice=0,
        .ArraySize=2,
      },
    };
    hr = m_device->CreateDepthStencilView(
      m_depthStencil.get(), &dsvDesc, m_dsv.put());
    if (FAILED(hr)) {
      return false;
    }

    return true;
  }

  void SetRTV(ID3D11Texture2D* colorTexture,
              int width,
              int height,
              DXGI_FORMAT colorSwapchainFormat)
  {
    CD3D11_VIEWPORT viewport{
      (float)0,
      (float)0,
      (float)width,
      (float)height,
    };
    m_deviceContext->RSSetViewports(1, &viewport);

    CreateDepth(width, height);

    // Create RenderTargetView with the original swapchain format (swapchain
    // image is typeless).
    winrt::com_ptr<ID3D11RenderTargetView> renderTargetView;
    const CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(
      D3D11_RTV_DIMENSION_TEXTURE2DARRAY, colorSwapchainFormat);
    m_device->CreateRenderTargetView(
      colorTexture, &renderTargetViewDesc, renderTargetView.put());

    // Clear swapchain and depth buffer. NOTE: This will clear the entire render
    // target view, not just the specified view.
    constexpr DirectX::XMFLOAT4 renderTargetClearColor = {
      0.0f,
      0.0f,
      0.0f,
      0.0f,
    };
    m_deviceContext->ClearRenderTargetView(renderTargetView.get(),
                                           &renderTargetClearColor.x);
    float depthClearValue = 1.0f;
    m_deviceContext->ClearDepthStencilView(
      m_dsv.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depthClearValue, 0);
    // m_deviceContext->OMSetDepthStencilState(nullptr, 0);

    ID3D11RenderTargetView* renderTargets[] = { renderTargetView.get() };
    m_deviceContext->OMSetRenderTargets(
      (UINT)std::size(renderTargets), renderTargets, m_dsv.get());
  }

  void Render(XrTime time,
              const XrSwapchainImageBaseHeader* swapchainImage,
              const XrfwSwapchains& info,
              const float projection[16],
              const float view[16],
              const float rightProjection[16],
              const float rightView[16])
  {
    // update
    m_instances.clear();
    auto space = xrfwAppSpace();
    for (auto& joint : m_trackerL->Update(time, space)) {
      auto size = joint.radius * 2;
      auto s = DirectX::XMMatrixScaling(size, size, size);
      auto r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(
        (const DirectX::XMFLOAT4*)&joint.pose.orientation));
      auto t = DirectX::XMMatrixTranslationFromVector(
        DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)&joint.pose.position));

      m_instances.push_back({});
      DirectX::XMStoreFloat4x4(&m_instances.back().Matrix, s * r * t);
    }
    for (auto& joint : m_trackerR->Update(time, space)) {
      auto size = joint.radius * 2;
      auto s = DirectX::XMMatrixScaling(size, size, size);
      auto r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(
        (const DirectX::XMFLOAT4*)&joint.pose.orientation));
      auto t = DirectX::XMMatrixTranslationFromVector(
        DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)&joint.pose.position));

      m_instances.push_back({});
      DirectX::XMStoreFloat4x4(&m_instances.back().Matrix, s * r * t);
    }

    SetRTV(xrfwCastTextureD3D11(swapchainImage),
           info.width,
           info.height,
           (DXGI_FORMAT)info.format);

    // render
    m_texture->Bind(m_deviceContext, 0);
    m_texture->Bind(m_deviceContext, 1);
    m_texture->Bind(m_deviceContext, 2);
    m_texture->Bind(m_deviceContext, 3);

    graphics.Render(projection,
                    view,
                    rightProjection,
                    rightView,
                    m_instances.data(),
                    m_instances.size());
  }

  static const XrCompositionLayerBaseHeader* Render(
    XrTime time,
    const XrSwapchainImageBaseHeader* swapchainImage,
    const XrSwapchainImageBaseHeader*,
    const XrfwSwapchains& info,
    const float projection[16],
    const float view[16],
    const float rightProjection[16],
    const float rightView[16],
    void* user)
  {
    auto context = ((Context*)user);
    context->Render(
      time, swapchainImage, info, projection, view, rightProjection, rightView);

    return (const XrCompositionLayerBaseHeader*)&context->m_passthroughFeature
      ->m_passthroughCompLayer;
  }

  static void Begin(XrSession session, void* user)
  {
    auto context = ((Context*)user);
    context->m_trackerL =
      std::make_shared<ExtHandTracker>(context->m_ext, session, true);
    context->m_trackerR =
      std::make_shared<ExtHandTracker>(context->m_ext, session, false);
    context->m_passthroughFeature =
      std::make_shared<FBPassthroughFeature>(context->m_passthrough, session);
    context->m_passthroughFeature->CreateLayer();
  }

  static void End(XrSession session, void* user)
  {
    auto context = ((Context*)user);
    context->m_trackerL = {};
    context->m_trackerR = {};
  }
};

int
main(int argc, char** argv)
{
  // instance
  XrfwPlatformWin32D3D11 platform;
  std::vector<const char*> extensions;
  for (auto& extension : ExtHandTracking::extensions()) {
    extensions.push_back(extension);
  }
  for (auto& extension : FBPassthrough::extensions()) {
    extensions.push_back(extension);
  }

  auto [instance, system] = platform.CreateInstance(extensions);
  if (!instance) {
    return 1;
  }
  auto device = platform.InitializeGraphics();
  if (!device) {
    return 2;
  }

  // session
  Context context(instance, system, device);
  auto ret = xrfwSession(
    platform, &Context::Render, &context, &Context::Begin, &Context::End);

  xrfwDestroyInstance();
  return ret;
}
