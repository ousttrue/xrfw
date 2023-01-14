#include <plog/Log.h>
#include <xrfw.h>

#include "oglrenderer.h"
template <typename T> int run(T &platform) {
  OglRenderer renderer;
  auto renderFunc = [](const XrSwapchainImageBaseHeader *swapchainImage,
                       int width, int height, const float projection[16],
                       const float view[16], void *user) {
    ((OglRenderer *)user)
        ->Render(xrfwCastTextureWin32OpenGL(swapchainImage), width, height,
                 projection, view);
  };
  return xrfwSession(platform, renderFunc, &renderer);
}

#ifdef XR_USE_PLATFORM_WIN32
int main(int argc, char **argv) {
  XrfwPlatformWin32OpenGL platform;
  auto instance = platform.CreateInstance();
  if (!instance) {
    return 1;
  }

  if (!platform.InitializeGraphics()) {
    return 2;
  }

  auto ret = run(platform);

  xrfwDestroyInstance();
  return ret;
}
#elif XR_USE_PLATFORM_ANDROID
#include <android_native_app_glue.h>
void android_main(struct android_app *state) {
  XrfwPlatform platform(state);
  auto instance = platform.CreateInstance();
  if (!instance) {
    return;
  }

  if (!platform.InitializeGraphics()) {
    return;
  }

  run(platform);

  xrfwDestroyInstance();
  return;
}
#else
error("XR_USE_PLATFORM_WIN32 or XR_USE_PLATFORM_ANDROID required")
#endif
