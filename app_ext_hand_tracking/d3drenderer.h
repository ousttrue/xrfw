#pragma once
#include <cuber/dx/DxCubeStereoRenderer.h>
#include <d3d11.h>
#include <winrt/base.h>
#include <xrfw.h>

struct D3DRenderer
{
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;
  winrt::com_ptr<ID3D11Texture2D> m_depthStencil;
  winrt::com_ptr<ID3D11DepthStencilView> m_dsv;
  cuber::dx11::DxCubeStereoRenderer m_cuber;
  D3DRenderer(const winrt::com_ptr<ID3D11Device>& device)
    : m_device(device)
    , m_cuber(device)
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

  void Render(const XrSwapchainImageBaseHeader* swapchainImage,
              const XrfwSwapchains& info,
              const float leftProjection[16],
              const float leftView[16],
              const float rightProjection[16],
              const float rightView[16],
              std::span<const cuber::Instance> instances)
  {
    SetRTV(xrfwCastTextureD3D11(swapchainImage),
           info.width,
           info.height,
           (DXGI_FORMAT)info.format);
    m_cuber.Render(leftProjection,
                   leftView,
                   rightProjection,
                   rightView,
                   instances.data(),
                   instances.size());
  }
};
