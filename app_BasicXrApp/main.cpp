#include <cuber/dx/DxCubeStereoRenderer.h>
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>

struct Cube
{
  DirectX::XMFLOAT3 Scale{ 0.1f, 0.1f, 0.1f };
  DirectX::XMFLOAT4 Rotation{ 0, 0, 0, 1 };
  DirectX::XMFLOAT3 Translation{ 0, 0, 0 };
  DirectX::XMFLOAT4 Color{ 1, 1, 1, 1 };
  void StoreInstance(cuber::Instance* instance) const
  {
    auto trs =
      DirectX::XMMatrixTransformation({},
                                      {},
                                      DirectX::XMLoadFloat3(&Scale),
                                      {},
                                      DirectX::XMLoadFloat4(&Rotation),
                                      DirectX::XMLoadFloat3(&Translation));
    DirectX::XMStoreFloat4x4(&instance->Matrix, trs);
    instance->PositiveFaceFlag = { 0, 1, 2, 0 };
    instance->NegativeFaceFlag = { 3, 4, 5, 0 };
  }
};

struct Context
{
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;

  cuber::dx11::DxCubeStereoRenderer m_cuber;
  std::vector<Cube> visible_cubes;
  std::vector<cuber::Instance> instances;

  Context(const winrt::com_ptr<ID3D11Device>& device)
    : m_device(device)
    , m_cuber(device)
  {
    m_device->GetImmediateContext(m_deviceContext.put());
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
    // m_deviceContext->ClearDepthStencilView(
    //     depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
    //     depthClearValue, 0);
    // m_deviceContext->OMSetDepthStencilState(
    //     reversedZ ? m_reversedZDepthNoStencilTest.get() : nullptr, 0);

    ID3D11RenderTargetView* renderTargets[] = { renderTargetView.get() };
    m_deviceContext->OMSetRenderTargets((UINT)std::size(renderTargets),
                                        renderTargets,
                                        // depthStencilView.get()
                                        nullptr);
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

    context->instances.clear();
    for (auto& cube : context->visible_cubes) {
      context->instances.push_back({});
      cube.StoreInstance(&context->instances.back());
    }
    context->SetRTV(xrfwCastTextureD3D11(swapchainImage),
                    info.width,
                    info.height,
                    (DXGI_FORMAT)info.format);
    context->m_cuber.Render(projection,
                                  view,
                                  rightProjection,
                                  rightView,
                                  context->instances.data(),
                                  context->instances.size());

    return nullptr;
  }
};

int
main(int argc, char** argv)
{
  XrfwPlatformWin32D3D11 platform;
  auto [instance, systemid] = platform.CreateInstance({});
  if (!instance) {
    return 1;
  }

  auto device = platform.InitializeGraphics();
  if (!device) {
    return 2;
  }

  // cubes
  Context context(device);
  context.visible_cubes.push_back({
    .Color = { 1, 0, 0, 1 },
  });
  context.visible_cubes.push_back({
    .Translation = { 1, 0, 0 },
    .Color = { 0, 0, 1, 1 },
  });

  auto ret = xrfwSession(platform, &Context::Render, &context);

  xrfwDestroyInstance();
  return ret;
}
