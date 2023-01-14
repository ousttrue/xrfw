#include "pch.h"

#include <plog/Log.h>
#include <xrfw.h>

#include "OpenXrProgram.h"

int main(int argc, char **argv) {
  XrfwPlatformWin32D3D11 platform;
  auto instance = platform.CreateInstance();
  if (!instance) {
    return 1;
  }

  if (!platform.InitializeGraphics()) {
    return 2;
  }

  auto graphics = sample::CreateCubeGraphics();
  auto renderFunc = [](const XrSwapchainImageBaseHeader *swapchainImage,
                       const XrSwapchainImageBaseHeader *,
                       const XrfwSwapchains &info, const float projection[16],
                       const float view[16], const float rightProjection[16],
                       const float rightView[16], void *user) {
    ((sample::IGraphicsPluginD3D11 *)user)
        ->RenderView(xrfwCastTextureD3D11(swapchainImage),
                     (DXGI_FORMAT)info.format, info.width, info.height,
                     projection, view, rightProjection, rightView);
  };
  auto ret = xrfwSession(platform, renderFunc, graphics.get());

  xrfwDestroyInstance();
  return ret;
}
