#include <plog/Log.h>
#include <xrfw.h>

#include "platform.h"

int xrSession(Platform &platform) {
  // session and swapchains from graphics
  XrfwSwapchains swapchains;
  auto session = platform.CreateSession(&swapchains);
  // auto session = xrfwCreateSession(&swapchains, platform.GraphicsBinding());
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
  return 0;
}

#ifdef XR_USE_PLATFORM_WIN32
int main(int argc, char **argv) {
  XrfwInitialization init;
  xrfwPlatformWin32OpenGL(&init);
  auto instance = xrfwCreateInstance(&init);
  if (!instance) {
    return 1;
  }

  Platform platform(nullptr);
  if (!platform.InitializeGraphics()) {
    return 2;
  }

  auto ret = xrSession(platform);
  xrfwDestroyInstance();
  return ret;
}
#elif XR_USE_PLATFORM_ANDROID
#include <android_native_app_glue.h>
void android_main(struct android_app *state) {
  if (!xrfwInitializeLoaderAndroid(state)) {
    return;
  }

  XrfwInitialization init;
  xrfwPlatformAndroidOpenGLES(&init, state);
  auto instance = xrfwCreateInstance(&init);
  if (!instance) {
    return;
  }

  Platform platform(state);
  if (!platform.InitializeGraphics()) {
    return;
  }

  xrSession(platform);
  xrfwDestroyInstance();
  return;
}
#else
error("XR_USE_PLATFORM_WIN32 or XR_USE_PLATFORM_ANDROID required")
#endif
