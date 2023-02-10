#include "CubeGraphics.h"
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
  sample::CubeGraphics graphics;
  std::vector<Cube> visible_cubes;
  std::vector<DirectX::XMFLOAT4X4> instances;

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
    context->graphics.RenderView(xrfwCastTextureD3D11(swapchainImage),
                                 (DXGI_FORMAT)info.format, info.width,
                                 info.height, projection, view, rightProjection,
                                 rightView, context->instances);
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

  Context context{
      .graphics = sample::CubeGraphics(device),
  };
  context.visible_cubes.push_back({});
  context.visible_cubes.push_back({});
  context.visible_cubes.back().Translation = {1, 0, 0};

  auto ret = xrfwSession(platform, &Context::Render, &context);

  xrfwDestroyInstance();
  return ret;
}
