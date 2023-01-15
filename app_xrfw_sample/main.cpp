#include <array>
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_swapchain_fbo.h>

#include "ogldrawable.h"
template <typename T> int run(T &platform) {

  struct Context {
    std::array<float, 4> clearColor = {0, 0, 0, 0};
    T &platform;
    std::shared_ptr<OglDrawable> drawable;
    XrfwSwapchainFbo fbo = {};
    std::vector<Cube> cubes;
  };
  Context context{
      .platform = platform,
      .drawable = OglDrawable::Create(),
  };
  // For each locatable space that we want to visualize, render a 25cm
  context.cubes.push_back(Cube{
      {0, 0, 0, 1},
      {0, 0, 0},
      {0.25f, 0.25f, 0.25f},
  });

  auto renderFunc = [](const XrSwapchainImageBaseHeader *swapchainImage,
                       const XrSwapchainImageBaseHeader *rightSwapchainImage,
                       const XrfwSwapchains &info, const float projection[16],
                       const float view[16], const float rightProjection[16],
                       const float rightView[16], void *user) {
    auto pContext = ((Context *)user);
    pContext->fbo.Begin(pContext->platform.CastTexture(swapchainImage),
                        info.width, info.height, &pContext->clearColor[0]);
    pContext->drawable->Render(projection, view, pContext->cubes);
    pContext->fbo.End();
    if (rightProjection) {
      pContext->fbo.Begin(pContext->platform.CastTexture(rightSwapchainImage),
                          info.width, info.height, &pContext->clearColor[0]);
      pContext->drawable->Render(rightProjection, rightView, pContext->cubes);
      pContext->fbo.End();
    }
  };
  return xrfwSession(platform, renderFunc, &context);
}

#ifdef XR_USE_PLATFORM_WIN32
#include <xrfw_impl_win32_opengl.h>
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
#include <xrfw_impl_android_opengl_es.h>
// #include <android_native_app_glue.h>
void android_main(struct android_app *state) {
  XrfwPlatformAndroidOpenGLES platform(state);
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
