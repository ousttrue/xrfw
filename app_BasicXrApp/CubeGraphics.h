#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <span>
#include <winrt/base.h> // winrt::com_ptr

namespace sample {

struct Cube {
  DirectX::XMFLOAT3 Scale{0.1f, 0.1f, 0.1f};
  DirectX::XMFLOAT4 Rotation{0, 0, 0, 1};
  DirectX::XMFLOAT3 Translation{0, 0, 0};
  void StoreMatrix(DirectX::XMFLOAT4X4 *m) const;
};

class CubeGraphics {
public:
  CubeGraphics(winrt::com_ptr<ID3D11Device> device);
  CubeGraphics(const CubeGraphics &) = delete;
  CubeGraphics &operator=(const CubeGraphics &) = delete;
  void RenderView(ID3D11Texture2D *colorTexture,
                  DXGI_FORMAT colorSwapchainFormat, int width, int height,
                  const float projection[16], const float view[16],
                  const float rightProjection[16], const float rightView[16],
                  std::span<Cube *> cubes);

private:
  void InitializeD3DResources();
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;
  winrt::com_ptr<ID3D11VertexShader> m_vertexShader;
  winrt::com_ptr<ID3D11PixelShader> m_pixelShader;
  winrt::com_ptr<ID3D11InputLayout> m_inputLayout;
  winrt::com_ptr<ID3D11Buffer> m_modelCBuffer;
  winrt::com_ptr<ID3D11Buffer> m_viewProjectionCBuffer;
  winrt::com_ptr<ID3D11Buffer> m_cubeVertexBuffer;
  winrt::com_ptr<ID3D11Buffer> m_cubeIndexBuffer;
  winrt::com_ptr<ID3D11DepthStencilState> m_reversedZDepthNoStencilTest;
};

} // namespace sample