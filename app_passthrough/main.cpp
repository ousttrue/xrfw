#include "../app_ext_hand_tracking/xr_ext_hand_tracking.h"
#include "desktop_capture.h"
#include "dxgi_util.h"
#include "xr_fb_paththrough.h"
#include <cuber/dx/DxCubeStereoRenderer.h>
#include <grapho/dx11/texture.h>
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>

const auto TextureBind = 0;
const auto PalleteIndex = 7;

struct Context
{
  static const int MAX_NUM_EYES = 2;
  DesktopCapture m_capture;

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
  FBPassthrough m_passthrough;
  std::shared_ptr<FBPassthroughFeature> m_passthroughFeature;

  winrt::com_ptr<ID3D11Texture2D> m_texture;
  winrt::com_ptr<ID3D11ShaderResourceView> m_shared;
  winrt::com_ptr<ID3D11SamplerState> m_sampler;

  Context(XrInstance instance,
          XrSystemId system,
          const winrt::com_ptr<ID3D11Device>& device)
    : m_device(device)
    , m_cuber(device)
    , m_ext(instance, system)
    , m_passthrough(instance, system)
  {
    m_device->GetImmediateContext(m_deviceContext.put());

    // pallete
    m_cuber.Pallete.Colors[PalleteIndex] = { 1, 1, 1, 1 };
    m_cuber.Pallete.Textures[PalleteIndex] = {
      TextureBind, TextureBind, TextureBind, 0
    };
    m_cuber.UploadPallete();

    // EnumOutput(m_device);

    auto obj = m_device.as<IDXGIObject>();
    winrt::com_ptr<IDXGIFactory1> factory;
    winrt::com_ptr<IDXGIAdapter1> adapter;
    obj->GetParent(IID_PPV_ARGS(adapter.put()));
    // auto factory = m_device.as<IDXGIFactory1>();
    // for (UINT adapterIndex = 0;
    //      S_OK == factory->EnumAdapters1(adapterIndex, adapter.put());
    //      ++adapterIndex) {
    // DXGI_ADAPTER_DESC1 desc;
    // adapter->GetDesc1(&desc);

    winrt::com_ptr<IDXGIOutput> output;
    for (UINT outputIndex = 0;
         S_OK == adapter->EnumOutputs(outputIndex, output.put());
         ++outputIndex, output.detach()) {

      // DXGI_OUTPUT_DESC output_desc;
      // output->GetDesc(&output_desc);
      //
      // PLOG_INFO << "output[" << outputIndex << "] " << output_desc.DeviceName
      //           << " "
      //           << (output_desc.DesktopCoordinates.right -
      //               output_desc.DesktopCoordinates.left)
      //           << ": "
      //           << (output_desc.DesktopCoordinates.bottom -
      //               output_desc.DesktopCoordinates.top);

      // texture
      auto handle = m_capture.Start(output);
      if (handle) {
        winrt::com_ptr<ID3D11Resource> resource;
        auto hr =
          m_device->OpenSharedResource(handle, IID_PPV_ARGS(resource.put()));
        if (SUCCEEDED(hr)) {
          m_texture = resource.as<ID3D11Texture2D>();
          if (m_texture) {
            D3D11_TEXTURE2D_DESC desc;
            m_texture->GetDesc(&desc);

            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {
              .Format = desc.Format,
              .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
              .Texture2D{
                .MostDetailedMip = 0,
                .MipLevels = desc.MipLevels,
              },
            };
            hr = m_device->CreateShaderResourceView(
              m_texture.get(), &srv_desc, m_shared.put());
            if (FAILED(hr)) {
              auto a = 0;
            }
          }
        }
      }
    }
    //   break;
    // }

    D3D11_SAMPLER_DESC sampler_desc = {
      .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
      .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
      .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
      .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
    };
    m_device->CreateSamplerState(&sampler_desc, m_sampler.put());
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

    // Clear swapchain and depth buffer. NOTE: This will clear the entire
    // render target view, not just the specified view.
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

    // cube
    {
      m_instances.push_back({});
      auto t = DirectX::XMMatrixTranslation(0, 1, -0.5f);
      auto s = DirectX::XMMatrixScaling(0.80f, 0.45f, 0.1f);
      DirectX::XMStoreFloat4x4(&m_instances.back().Matrix, s * t);
    }
    m_instances.back().PositiveFaceFlag = {
      PalleteIndex, PalleteIndex, PalleteIndex, 0
    };
    m_instances.back().NegativeFaceFlag = {
      PalleteIndex, PalleteIndex, PalleteIndex, 0
    };

    SetRTV(xrfwCastTextureD3D11(swapchainImage),
           info.width,
           info.height,
           (DXGI_FORMAT)info.format);

    // render
    ID3D11ShaderResourceView* srvs[] = {
      m_shared.get(),
      m_shared.get(),
      m_shared.get(),
    };
    m_deviceContext->PSSetShaderResources(0, std::size(srvs), srvs);

    ID3D11SamplerState* samplers[] = {
      m_sampler.get(),
      m_sampler.get(),
      m_sampler.get(),
    };
    m_deviceContext->PSSetSamplers(0, std::size(samplers), samplers);

    m_cuber.Render(projection,
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
