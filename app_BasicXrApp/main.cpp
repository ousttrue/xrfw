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
                       const XrSwapchainImageBaseHeader *, int width,
                       int height, const float projection[16],
                       const float view[16], const float rightProjection[16],
                       const float rightView[16], void *user) {
    ((sample::IGraphicsPluginD3D11 *)user)
        ->Render(xrfwCastTextureD3D11(swapchainImage), width, height,
                 projection, view);
  };
  auto ret = xrfwSession(platform, renderFunc, graphics.get());

  xrfwDestroyInstance();
  return ret;
}
