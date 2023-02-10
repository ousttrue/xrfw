#include "CubeGraphics.h"
#include <XrUtility/XrError.h>
#include <d3dcompiler.h>

const uint32_t viewInstanceCount = 2; // (uint32_t)viewProjections.size();

namespace CubeShader {
struct Vertex {
  XrVector3f Position;
  XrVector3f Color;
};

constexpr XrVector3f Red{1, 0, 0};
constexpr XrVector3f DarkRed{0.25f, 0, 0};
constexpr XrVector3f Green{0, 1, 0};
constexpr XrVector3f DarkGreen{0, 0.25f, 0};
constexpr XrVector3f Blue{0, 0, 1};
constexpr XrVector3f DarkBlue{0, 0, 0.25f};

// Vertices for a 1x1x1 meter cube. (Left/Right, Top/Bottom, Front/Back)
constexpr XrVector3f LBB{-0.5f, -0.5f, -0.5f};
constexpr XrVector3f LBF{-0.5f, -0.5f, 0.5f};
constexpr XrVector3f LTB{-0.5f, 0.5f, -0.5f};
constexpr XrVector3f LTF{-0.5f, 0.5f, 0.5f};
constexpr XrVector3f RBB{0.5f, -0.5f, -0.5f};
constexpr XrVector3f RBF{0.5f, -0.5f, 0.5f};
constexpr XrVector3f RTB{0.5f, 0.5f, -0.5f};
constexpr XrVector3f RTF{0.5f, 0.5f, 0.5f};

#define CUBE_SIDE(V1, V2, V3, V4, V5, V6, COLOR)                               \
  {V1, COLOR}, {V2, COLOR}, {V3, COLOR}, {V4, COLOR}, {V5, COLOR}, {V6, COLOR},

constexpr Vertex c_cubeVertices[] = {
    CUBE_SIDE(LTB, LBF, LBB, LTB, LTF, LBF, DarkRed)   // -X
    CUBE_SIDE(RTB, RBB, RBF, RTB, RBF, RTF, Red)       // +X
    CUBE_SIDE(LBB, LBF, RBF, LBB, RBF, RBB, DarkGreen) // -Y
    CUBE_SIDE(LTB, RTB, RTF, LTB, RTF, LTF, Green)     // +Y
    CUBE_SIDE(LBB, RBB, RTB, LBB, RTB, LTB, DarkBlue)  // -Z
    CUBE_SIDE(LBF, LTF, RTF, LBF, RTF, RBF, Blue)      // +Z
};

// Winding order is clockwise. Each side uses a different color.
constexpr unsigned short c_cubeIndices[] = {
    0,  1,  2,  3,  4,  5,  // -X
    6,  7,  8,  9,  10, 11, // +X
    12, 13, 14, 15, 16, 17, // -Y
    18, 19, 20, 21, 22, 23, // +Y
    24, 25, 26, 27, 28, 29, // -Z
    30, 31, 32, 33, 34, 35, // +Z
};

struct ViewProjectionConstantBuffer {
  DirectX::XMFLOAT4X4 ViewProjection[2];
};

constexpr uint32_t MaxViewInstance = 2;

// Separate entrypoints for the vertex and pixel shader functions.
constexpr char ShaderHlsl[] = R"_(
#pragma pack_matrix(row_major)

cbuffer ViewProjectionConstantBuffer : register(b0) {
    float4x4 ViewProjection[2];
};
struct VSInput {
    float3 Pos : POSITION;
    float3 Color : COLOR0;
    float4 Row0: ROW0;
    float4 Row1: ROW1;
    float4 Row2: ROW2;
    float4 Row3: ROW3;
    uint instId : SV_InstanceID;
};
struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Color : COLOR0;
    uint viewId : SV_RenderTargetArrayIndex;
};

float4x4 transform(float4 r0, float4 r1, float4 r2, float4 r3)
{
  return float4x4(
    r0.x, r0.y, r0.z, r0.w,
    r1.x, r1.y, r1.z, r1.w,
    r2.x, r2.y, r2.z, r2.w,
    r3.x, r3.y, r3.z, r3.w
  );
}

VSOutput MainVS(VSInput IN) {
    VSOutput output;
    output.Pos = mul(mul(float4(IN.Pos, 1), transform(IN.Row0, IN.Row1, IN.Row2, IN.Row3)), ViewProjection[IN.instId % 2]);
    output.Color = IN.Color;
    output.viewId = IN.instId % 2;
    return output;
}

float4 MainPS(VSOutput IN) : SV_TARGET {
    return float4(IN.Color, 1);
}
)_";
} // namespace CubeShader

namespace sample {

static winrt::com_ptr<ID3DBlob> CompileShader(const char *hlsl,
                                              const char *entrypoint,
                                              const char *shaderTarget) {
  winrt::com_ptr<ID3DBlob> compiled;
  winrt::com_ptr<ID3DBlob> errMsgs;
  DWORD flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR |
                D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;

#ifdef _DEBUG
  flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
  flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

  HRESULT hr =
      D3DCompile(hlsl, strlen(hlsl), nullptr, nullptr, nullptr, entrypoint,
                 shaderTarget, flags, 0, compiled.put(), errMsgs.put());
  if (FAILED(hr)) {
    std::string errMsg((const char *)errMsgs->GetBufferPointer(),
                       errMsgs->GetBufferSize());
    DEBUG_PRINT("D3DCompile failed %X: %s", hr, errMsg.c_str());
    CHECK_HRESULT(hr, "D3DCompile failed");
  }

  return compiled;
}

CubeGraphics::CubeGraphics(winrt::com_ptr<ID3D11Device> device)
    : m_device(device) {
  m_device->GetImmediateContext(m_deviceContext.put());
  InitializeD3DResources();
}

void CubeGraphics::InitializeD3DResources() {
  // vs ps layout
  {
    const winrt::com_ptr<ID3DBlob> vertexShaderBytes =
        CompileShader(CubeShader::ShaderHlsl, "MainVS", "vs_5_0");
    CHECK_HRCMD(m_device->CreateVertexShader(
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(), nullptr, m_vertexShader.put()));

    const winrt::com_ptr<ID3DBlob> pixelShaderBytes =
        CompileShader(CubeShader::ShaderHlsl, "MainPS", "ps_5_0");
    CHECK_HRCMD(m_device->CreatePixelShader(
        pixelShaderBytes->GetBufferPointer(), pixelShaderBytes->GetBufferSize(),
        nullptr, m_pixelShader.put()));

    const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
        // Vertex
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_VERTEX_DATA,
            0,
        },
        {
            "COLOR",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_VERTEX_DATA,
            0,
        },
        // Instance
        {
            "ROW",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_INSTANCE_DATA,
            2,
        },
        {
            "ROW",
            1,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_INSTANCE_DATA,
            2,
        },
        {
            "ROW",
            2,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_INSTANCE_DATA,
            2,
        },
        {
            "ROW",
            3,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_INSTANCE_DATA,
            2,
        },
    };

    CHECK_HRCMD(m_device->CreateInputLayout(
        vertexDesc, (UINT)std::size(vertexDesc),
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(), m_inputLayout.put()));
  }

  // VP
  {
    const CD3D11_BUFFER_DESC viewProjectionConstantBufferDesc(
        sizeof(CubeShader::ViewProjectionConstantBuffer),
        D3D11_BIND_CONSTANT_BUFFER);
    CHECK_HRCMD(m_device->CreateBuffer(&viewProjectionConstantBufferDesc,
                                       nullptr, m_viewProjectionCBuffer.put()));
  }

  // vertex
  {
    const D3D11_SUBRESOURCE_DATA vertexBufferData{CubeShader::c_cubeVertices};
    const CD3D11_BUFFER_DESC vertexBufferDesc(
        sizeof(CubeShader::c_cubeVertices), D3D11_BIND_VERTEX_BUFFER);
    CHECK_HRCMD(m_device->CreateBuffer(&vertexBufferDesc, &vertexBufferData,
                                       m_cubeVertexBuffer.put()));
  }
  // instance
  {
    const CD3D11_BUFFER_DESC vertexBufferDesc(
        sizeof(DirectX::XMFLOAT4X4) * 65535, D3D11_BIND_VERTEX_BUFFER);
    CHECK_HRCMD(m_device->CreateBuffer(&vertexBufferDesc, nullptr,
                                       m_cubeInstanceBuffer.put()));
  }

  // index
  {
    const D3D11_SUBRESOURCE_DATA indexBufferData{CubeShader::c_cubeIndices};
    const CD3D11_BUFFER_DESC indexBufferDesc(sizeof(CubeShader::c_cubeIndices),
                                             D3D11_BIND_INDEX_BUFFER);
    CHECK_HRCMD(m_device->CreateBuffer(&indexBufferDesc, &indexBufferData,
                                       m_cubeIndexBuffer.put()));
  }

  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS3 options;
    m_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &options,
                                  sizeof(options));
    CHECK_MSG(options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer,
              "This sample requires VPRT support. Adjust sample shaders on GPU "
              "without VRPT.");
  }

  {
    CD3D11_DEPTH_STENCIL_DESC depthStencilDesc(CD3D11_DEFAULT{});
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;
    CHECK_HRCMD(m_device->CreateDepthStencilState(
        &depthStencilDesc, m_reversedZDepthNoStencilTest.put()));
  }
}

void CubeGraphics::SetRTV(ID3D11Texture2D *colorTexture, int width, int height,
                          DXGI_FORMAT colorSwapchainFormat) {
  CHECK_MSG(viewInstanceCount <= CubeShader::MaxViewInstance,
            "Sample shader supports 2 or fewer view instances. Adjust shader "
            "to accommodate more.")

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
  CHECK_HRCMD(m_device->CreateRenderTargetView(
      colorTexture, &renderTargetViewDesc, renderTargetView.put()));

  // Clear swapchain and depth buffer. NOTE: This will clear the entire render
  // target view, not just the specified view.
  constexpr DirectX::XMVECTORF32 renderTargetClearColor = {
      0.184313729f,
      0.309803933f,
      0.309803933f,
      1.000000000f,
  };

  m_deviceContext->ClearRenderTargetView(renderTargetView.get(),
                                         renderTargetClearColor);
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

void CubeGraphics::RenderView(const float projection[16], const float view[16],
                              const float rightProjection[16],
                              const float rightView[16],
                              std::span<DirectX::XMFLOAT4X4> cubes) {

  ID3D11Buffer *const constantBuffers[] = {
      m_viewProjectionCBuffer.get(),
  };
  m_deviceContext->VSSetConstantBuffers(0, (UINT)std::size(constantBuffers),
                                        constantBuffers);
  m_deviceContext->VSSetShader(m_vertexShader.get(), nullptr, 0);
  m_deviceContext->PSSetShader(m_pixelShader.get(), nullptr, 0);

  CubeShader::ViewProjectionConstantBuffer viewProjectionCBufferData{};
  for (uint32_t k = 0; k < viewInstanceCount; k++) {
    const DirectX::XMMATRIX spaceToView = DirectX::XMLoadFloat4x4(
        (DirectX::XMFLOAT4X4 *)(k == 0 ? view : rightView));
    const DirectX::XMMATRIX projectionMatrix = DirectX::XMLoadFloat4x4(
        (DirectX::XMFLOAT4X4 *)(k == 0 ? projection : rightProjection));
    DirectX::XMStoreFloat4x4(&viewProjectionCBufferData.ViewProjection[k],
                             spaceToView * projectionMatrix);
  }
  m_deviceContext->UpdateSubresource(m_viewProjectionCBuffer.get(), 0, nullptr,
                                     &viewProjectionCBufferData, 0, 0);

  // Set cube primitive data.
  const UINT strides[] = {
      sizeof(CubeShader::Vertex),
      sizeof(DirectX::XMFLOAT4X4),
  };
  const UINT offsets[] = {
      0,
      0,
  };
  ID3D11Buffer *vertexBuffers[] = {
      m_cubeVertexBuffer.get(),
      m_cubeInstanceBuffer.get(),
  };
  m_deviceContext->IASetVertexBuffers(0, (UINT)std::size(vertexBuffers),
                                      vertexBuffers, strides, offsets);
  m_deviceContext->IASetIndexBuffer(m_cubeIndexBuffer.get(),
                                    DXGI_FORMAT_R16_UINT, 0);
  m_deviceContext->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_deviceContext->IASetInputLayout(m_inputLayout.get());

  // Render each cube

  // Compute and update the model transform for each cube, transpose for
  // shader usage.
  //   CubeShader::ModelConstantBuffer model;
  //   cube->StoreMatrix(&model.Model);

  D3D11_BOX box{
      .left = 0,
      .top = 0,
      .front = 0,
      .right =
          static_cast<uint32_t>(sizeof(DirectX::XMFLOAT4X4) * cubes.size()),
      .bottom = 1,
      .back = 1,
  };
  m_deviceContext->UpdateSubresource(m_cubeInstanceBuffer.get(), 0, &box,
                                     cubes.data(), 0, 0);

  // Draw the cube.
  m_deviceContext->DrawIndexedInstanced(
      (UINT)std::size(CubeShader::c_cubeIndices),
      viewInstanceCount * cubes.size(), 0, 0, 0);
}

} // namespace sample