#pragma once
#include <XrUtility/XrError.h>
#include <XrUtility/XrHandle.h>
#include <XrUtility/XrMath.h>
#include <XrUtility/XrString.h>
#include <d3d11.h>
#include <openxr/openxr.h>
#include <winrt/base.h> // winrt::com_ptr

namespace sample {

struct Cube {
  xr::SpaceHandle Space{};
  std::optional<XrPosef>
      PoseInSpace{}; // Relative pose in cube Space. Default to identity.
  XrVector3f Scale{0.1f, 0.1f, 0.1f};

  XrPosef PoseInAppSpace =
      xr::math::Pose::Identity(); // Cube pose in app space that gets updated
                                  // every frame
};

class CubeGraphics {

public:
  CubeGraphics(winrt::com_ptr<ID3D11Device> device);
  CubeGraphics(const CubeGraphics &) = delete;
  CubeGraphics &operator=(const CubeGraphics &) = delete;
  void RenderView(ID3D11Texture2D *colorTexture,
                  DXGI_FORMAT colorSwapchainFormat, int width, int height,
                  const float projection[16], const float view[16],
                  const float rightProjection[16], const float rightView[16]);

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