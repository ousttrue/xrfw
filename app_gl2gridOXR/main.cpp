#include <plog/Log.h>
#include <xrfw.h>

#include "render_scene.h"
template <typename T> int run(T &platform) {
  init_gles_scene();
  auto renderFunc = [](const XrSwapchainImageBaseHeader *swapchainImage,
                       const XrSwapchainImageBaseHeader *rightSwapchainImage,
                       int width, int height, const float projection[16],
                       const float view[16], const float rightProjection[16],
                       const float rightView[16], void *user) {
    render_gles_scene(xrfwCastTextureWin32OpenGL(swapchainImage), width, height,
                      projection, view);
    if (rightProjection) {
      render_gles_scene(xrfwCastTextureWin32OpenGL(rightSwapchainImage), width,
                        height, rightProjection, rightView);
    }
  };
  return xrfwSession(platform, renderFunc, nullptr);
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
