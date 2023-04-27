#include "../app_ext_hand_tracking/xr_ext_hand_tracking.h"
#include <cuber/dx/DxCubeStereoRenderer.h>
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>
#include <xrfw_proc.h>

struct FBPassthrough
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_FB_PASSTHROUGH_EXTENSION_NAME,
      XR_FB_TRIANGLE_MESH_EXTENSION_NAME // optional
    };
    return s_extensions;
  }
  static bool SystemSupportsPassthrough(XrInstance instance,
                                        XrFormFactor formFactor)
  {
    XrSystemPassthroughProperties2FB passthroughSystemProperties{
      XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES2_FB
    };
    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES,
                                         &passthroughSystemProperties };

    XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
    systemGetInfo.formFactor = formFactor;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance, &systemGetInfo, &systemId);
    xrGetSystemProperties(instance, systemId, &systemProperties);

    // XR_PASSTHROUGH_CAPABILITY_COLOR_BIT_FB
    return (passthroughSystemProperties.capabilities &
            XR_PASSTHROUGH_CAPABILITY_BIT_FB) ==
           XR_PASSTHROUGH_CAPABILITY_BIT_FB;
  }

  XRFW_PROC(xrCreatePassthroughFB);
  XRFW_PROC(xrCreatePassthroughLayerFB);
  XRFW_PROC(xrPassthroughLayerSetStyleFB);
  FBPassthrough(XrInstance instance, XrSystemId system)
    : xrCreatePassthroughFB(instance)
    , xrCreatePassthroughLayerFB(instance)
    , xrPassthroughLayerSetStyleFB(instance)
  {
  }
};

struct FBPassthroughFeature
{
  const FBPassthrough& m_ext;
  XrSession m_session;
  XrPassthroughFB m_passthroughFeature = XR_NULL_HANDLE;
  XrPassthroughLayerFB m_passthroughLayer = XR_NULL_HANDLE;
  XrCompositionLayerPassthroughFB m_passthroughCompLayer = {};

  FBPassthroughFeature(const FBPassthrough& ext, XrSession session)
    : m_ext(ext)
    , m_session(session)
  {
    XrPassthroughCreateInfoFB passthroughCreateInfo = {
      .type = XR_TYPE_PASSTHROUGH_CREATE_INFO_FB,
      .flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB,
    };

    XrResult result = m_ext.xrCreatePassthroughFB(
      session, &passthroughCreateInfo, &m_passthroughFeature);
    if (XR_FAILED(result)) {
      PLOG_ERROR << "Failed to create a passthrough feature.";
    }
  }

  void CreateLayer()
  {
    // Create and run passthrough layer

    XrPassthroughLayerCreateInfoFB layerCreateInfo = {
      .type = XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB,
      .passthrough = m_passthroughFeature,
      .flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB,
      .purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB,
    };

    XrResult result = m_ext.xrCreatePassthroughLayerFB(
      m_session, &layerCreateInfo, &m_passthroughLayer);
    if (XR_FAILED(result)) {
      PLOG_ERROR << "Failed to create and start a passthrough layer";
    }

    m_passthroughCompLayer = {
      .type = XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB,
      .flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT,
      .space = XR_NULL_HANDLE,
      .layerHandle = m_passthroughLayer,
    };

    XrPassthroughStyleFB style{
      .type = XR_TYPE_PASSTHROUGH_STYLE_FB,
      .textureOpacityFactor = 0.5f,
      .edgeColor = { 0.0f, 0.0f, 0.0f, 0.0f },
      // .next = &colorMap,
    };
    if (FAILED(
          m_ext.xrPassthroughLayerSetStyleFB(m_passthroughLayer, &style))) {
      PLOG_ERROR << "xrPassthroughLayerSetStyleFB";
    }
  }

  // void Start()
  // {
  //   // Start the feature manually
  //   XrResult result = m_ext.pfnXrPassthroughStartFB(passthroughFeature);
  //   if (XR_FAILED(result)) {
  //     LOG("Failed to start a passthrough feature.\n");
  //   }
  // }
};

struct Context
{
  static const int MAX_NUM_EYES = 2;

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

  Context(XrInstance instance,
          XrSystemId system,
          const winrt::com_ptr<ID3D11Device>& device)
    : m_device(device)
    , graphics(device)
    , m_ext(instance, system)
    , m_passthrough(instance, system)
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
      m_instances.back().Color = { 1, 1, 1, 1 };
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
      m_instances.back().Color = { 1, 1, 1, 1 };
    }

    // render
    SetRTV(xrfwCastTextureD3D11(swapchainImage),
           info.width,
           info.height,
           (DXGI_FORMAT)info.format);
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
