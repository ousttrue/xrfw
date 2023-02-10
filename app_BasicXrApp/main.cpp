#include <cuber/DxCubeStereoRenderer.h>
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>

struct Cube {
  DirectX::XMFLOAT3 Scale{0.1f, 0.1f, 0.1f};
  DirectX::XMFLOAT4 Rotation{0, 0, 0, 1};
  DirectX::XMFLOAT3 Translation{0, 0, 0};
  void StoreMatrix(DirectX::XMFLOAT4X4 *m) const {
    auto trs = DirectX::XMMatrixTransformation(
        {}, {}, DirectX::XMLoadFloat3(&Scale), {},
        DirectX::XMLoadFloat4(&Rotation), DirectX::XMLoadFloat3(&Translation));
    DirectX::XMStoreFloat4x4(m, trs);
  }
};

struct Context {
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;

  cuber::DxCubeStereoRenderer graphics;
  std::vector<Cube> visible_cubes;
  std::vector<DirectX::XMFLOAT4X4> instances;

  Context(const winrt::com_ptr<ID3D11Device> &device)
      : m_device(device), graphics(device) {
    m_device->GetImmediateContext(m_deviceContext.put());
  }

  void SetRTV(ID3D11Texture2D *colorTexture, int width, int height,
              DXGI_FORMAT colorSwapchainFormat) {
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
    m_device->CreateRenderTargetView(colorTexture, &renderTargetViewDesc,
                                     renderTargetView.put());

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

    ID3D11RenderTargetView *renderTargets[] = {renderTargetView.get()};
    m_deviceContext->OMSetRenderTargets((UINT)std::size(renderTargets),
                                        renderTargets,
                                        // depthStencilView.get()
                                        nullptr);
  }

  static void Render(const XrSwapchainImageBaseHeader *swapchainImage,
                     const XrSwapchainImageBaseHeader *,
                     const XrfwSwapchains &info, const float projection[16],
                     const float view[16], const float rightProjection[16],
                     const float rightView[16], void *user) {
    auto context = ((Context *)user);

    context->instances.clear();
    for (auto &cube : context->visible_cubes) {
      context->instances.push_back({});
      cube.StoreMatrix(&context->instances.back());
    }
    context->SetRTV(xrfwCastTextureD3D11(swapchainImage), info.width,
                    info.height, (DXGI_FORMAT)info.format);
    context->graphics.Render<DirectX::XMFLOAT4X4>(
        projection, view, rightProjection, rightView, context->instances);
  }
};

int main(int argc, char **argv) {
  XrfwPlatformWin32D3D11 platform;
  auto instance = platform.CreateInstance();
  if (!instance) {
    return 1;
  }

  auto device = platform.InitializeGraphics();
  if (!device) {
    return 2;
  }

  // cubes
  Context context(device);
  context.visible_cubes.push_back({});
  context.visible_cubes.push_back({});
  context.visible_cubes.back().Translation = {1, 0, 0};

  auto ret = xrfwSession(platform, &Context::Render, &context);

  xrfwDestroyInstance();
  return ret;
}
