#include <plog/Log.h>
#include <xrfw.h>

#include "platform.h"

int start() {
  Platform platform;
  if (!platform.InitializeGraphics()) {
    return 1;
  }

  // instance
  auto instance = xrfwCreateInstance(platform.Extensions(), 1);
  if (!instance) {
    return 2;
  }

  // session and swapchains from graphics
  XrfwSwapchains swapchains;
  auto session = xrfwCreateSession(&swapchains, platform.GraphicsBinding());
  if (!session) {
    return 3;
  }

  OglRenderer renderer;

  // glfw mainloop
  while (platform.BeginFrame()) {

    // OpenXR handling
    if (xrfwPollEventsIsSessionActive()) {
      XrTime frameTime;
      XrfwViewMatrices viewMatrix;
      if (xrfwBeginFrame(&frameTime, &viewMatrix)) {
        // left
        if (auto swapchainImage = xrfwAcquireSwapchain(swapchains.left)) {
          auto colorTexture = platform.CastTexture(swapchainImage);
          renderer.Render(colorTexture, swapchains.leftWidth,
                          swapchains.leftHeight, viewMatrix.leftProjection,
                          viewMatrix.leftView);
          xrfwReleaseSwapchain(swapchains.left);
        }
        // right
        if (auto swapchainImage = xrfwAcquireSwapchain(swapchains.right)) {
          auto colorTexture = platform.CastTexture(swapchainImage);
          renderer.Render(colorTexture, swapchains.rightWidth,
                          swapchains.rightHeight, viewMatrix.rightProjection,
                          viewMatrix.rightView);
          xrfwReleaseSwapchain(swapchains.right);
        }
      }
      xrfwEndFrame();
    } else {
      // XrSession is not active
      platform.Sleep(std::chrono::milliseconds(30));
    }

    platform.EndFrame(renderer);
  }

  xrfwDestroySession(session);
  xrfwDestroyInstance();
  return 0;
}

#ifdef XR_USE_PLATFORM_WIN32
int main(int argc, char **argv) { return start(); }
#elif XR_USE_PLATFORM_ANDROID
void android_main(struct android_app *state) { start(); }
#else
error("XR_USE_PLATFORM_WIN32 or XR_USE_PLATFORM_ANDROID required")
#endif
