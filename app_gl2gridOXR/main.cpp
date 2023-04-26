#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_opengl.h>
#include <xrfw_swapchain_fbo.h>

#include "render_scene.h"

static float g_clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };

static void
renderFunc(XrTime time,
           const XrSwapchainImageBaseHeader* swapchainImage,
           const XrSwapchainImageBaseHeader* rightSwapchainImage,
           const XrfwSwapchains& info,
           const float projection[16],
           const float view[16],
           const float rightProjection[16],
           const float rightView[16],
           void* user)
{
  auto fbo = (XrfwSwapchainFbo*)user;
  fbo->Begin(xrfwCastTextureWin32OpenGL(swapchainImage),
             info.width,
             info.height,
             g_clearColor);
  render_gles_scene(info.width, info.height, projection, view);
  if (rightProjection) {
    fbo->Begin(xrfwCastTextureWin32OpenGL(rightSwapchainImage),
               info.width,
               info.height,
               g_clearColor);
    render_gles_scene(info.width, info.height, rightProjection, rightView);
  }
  fbo->End();
}

template<typename T>
int
run(T& platform)
{
  init_gles_scene();
  XrfwSwapchainFbo fbo;
  return xrfwSession(platform, renderFunc, &fbo);
}

#ifdef XR_USE_PLATFORM_WIN32
int
main(int argc, char** argv)
{
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
void
android_main(struct android_app* state)
{
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
