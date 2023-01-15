#include "pch.h"

#include "CubeGraphics.h"
#include <plog/Log.h>
#include <xrfw.h>

struct Context {
  sample::CubeGraphics graphics;
  std::vector<sample::Cube *> visible_cubes;
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
  sample::Cube origin = {};
  context.visible_cubes.push_back(&origin);

  auto renderFunc = [](const XrSwapchainImageBaseHeader *swapchainImage,
                       const XrSwapchainImageBaseHeader *,
                       const XrfwSwapchains &info, const float projection[16],
                       const float view[16], const float rightProjection[16],
                       const float rightView[16], void *user) {
    auto context = ((Context *)user);
    context->graphics.RenderView(xrfwCastTextureD3D11(swapchainImage),
                                 (DXGI_FORMAT)info.format, info.width,
                                 info.height, projection, view, rightProjection,
                                 rightView, context->visible_cubes);
  };
  auto ret = xrfwSession(platform, renderFunc, &context);

  xrfwDestroyInstance();
  return ret;
}
