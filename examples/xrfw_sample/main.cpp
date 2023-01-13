#include <plog/Log.h>
#include <xrfw.h>

#include "oglrenderer.h"

#ifdef XR_USE_PLATFORM_WIN32
int main(int argc, char **argv) {
  XrfwPlatform platform;
  auto instance = platform.CreateInstance();
  if (!instance) {
    return 1;
  }

  if (!platform.InitializeGraphics()) {
    return 2;
  }

  OglRenderer renderer;
  auto renderFunc = [](uint32_t colorTexture, int width, int height,
                       const float projection[16], const float view[16],
                       void *user) {
    ((OglRenderer *)user)
        ->Render(colorTexture, width, height, projection, view);
  };

  auto ret = xrfwSession(platform, renderFunc, &renderer);
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

  OglRenderer renderer;
  auto renderFunc = [](uint32_t colorTexture, int width, int height,
                       const float projection[16], const float view[16],
                       void *user) {
    ((OglRenderer *)user)
        ->Render(colorTexture, width, height, projection, view);
  };

  xrfwSession(platform, renderFunc, &renderer);
  xrfwDestroyInstance();
  return;
}
#else
error("XR_USE_PLATFORM_WIN32 or XR_USE_PLATFORM_ANDROID required")
#endif
