#include "xr_ext_hand_tracking.h"
#include <cuber/dx/DxCubeStereoRenderer.h>
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>

struct Context
{
  static const int MAX_NUM_EYES = 2;

  // d3d11
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;
  winrt::com_ptr<ID3D11Texture2D> m_depthStencil;
  winrt::com_ptr<ID3D11DepthStencilView> m_dsv;
  cuber::dx11::DxCubeStereoRenderer m_cuber;
  std::vector<cuber::Instance> m_instances;

  ExtHandTracking m_ext;
  std::shared_ptr<ExtHandTracker> m_trackerL;
  std::shared_ptr<ExtHandTracker> m_trackerR;

  Context(XrInstance instance,
          XrSystemId system,
          const winrt::com_ptr<ID3D11Device>& device)
    : m_device(device)
    , m_cuber(device)
    , m_ext(instance, system)
  {
    m_device->GetImmediateContext(m_deviceContext.put());
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
      0.184313729f,
      0.309803933f,
      0.309803933f,
      1.000000000f,
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

  void UpdateJoints(std::span<const XrHandJointLocationEXT> joints,
                    const DirectX::XMFLOAT4& color)
  {
    const XrSpaceLocationFlags isValid =
      XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
      XR_SPACE_LOCATION_POSITION_VALID_BIT;

    for (auto& joint : joints) {
      if ((joint.locationFlags & isValid) != 0) {
        auto size = joint.radius * 2;
        auto s = DirectX::XMMatrixScaling(size, size, size);
        auto r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(
          (const DirectX::XMFLOAT4*)&joint.pose.orientation));
        auto t = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(
          (const DirectX::XMFLOAT3*)&joint.pose.position));

        m_instances.push_back({});
        DirectX::XMStoreFloat4x4(&m_instances.back().Matrix, s * r * t);
        m_instances.back().Color = color;
      }
    }
  }

  void Render(XrTime time,
              const XrSwapchainImageBaseHeader* swapchainImage,
              const XrfwSwapchains& info,
              const float leftProjection[16],
              const float leftView[16],
              const float rightProjection[16],
              const float rightView[16])
  {
    // update
    m_instances.clear();
    auto space = xrfwAppSpace();
    UpdateJoints(m_trackerL->Update(time, space), { 1, 0, 0, 1 });
    UpdateJoints(m_trackerR->Update(time, space), { 0, 0, 1, 1 });

    // render
    SetRTV(xrfwCastTextureD3D11(swapchainImage),
           info.width,
           info.height,
           (DXGI_FORMAT)info.format);
    m_cuber.Render(leftProjection,
                   leftView,
                   rightProjection,
                   rightView,
                   m_instances.data(),
                   m_instances.size());
  }

  static void Render(XrTime time,
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
  }

  static void Begin(XrSession session, void* user)
  {
    auto context = ((Context*)user);
    context->m_trackerL =
      std::make_shared<ExtHandTracker>(context->m_ext, session, true);
    context->m_trackerR =
      std::make_shared<ExtHandTracker>(context->m_ext, session, false);
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
  auto [instance, system] =
    platform.CreateInstance(ExtHandTracking::extensions());
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
