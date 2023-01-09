#include <windows.h>
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <Glfw/glfw3native.h>

#include <plog/Log.h>

#include "oxrrenderer.h"
#include <gl/GL.h>
#include <xrfw.h>

int main(int argc, char **argv) {
  // OpenGL context
  if (!glfwInit()) {
    return -1;
  }

  // Create a windowed mode window and its OpenGL context
  auto window = glfwCreateWindow(640, 480, "Hello xrfw", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -2;
  }

  // require openxr graphics extension
  const char *extensions[] = {
      XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
  };
  auto instance = xrfwCreateInstance(extensions, 1);
  if (!instance) {
    return 1;
  }

  // Make the window's context current
  glfwMakeContextCurrent(window);
  PLOG_INFO << glGetString(GL_VERSION);

  auto session = xrfwCreateOpenGLWin32Session(GetDC(glfwGetWin32Window(window)),
                                              glfwGetWGLContext(window));
  if (!session) {
    return 2;
  }

  XrViewConfigurationView viewConfigurationViews[2];
  if(!xrfwGetViewConfigurationViews(viewConfigurationViews, 2))
  {
    return 3;
  }

  OxrRenderer oxr(instance, session);
  if (!oxr.CreateSwapchain(viewConfigurationViews[0])) {
    return 4;
  }
  if (!oxr.CreateSwapchain(viewConfigurationViews[1])) {
    return 5;
  }

  // glfw mainloop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if (xrfwPollEventsAndIsActive()) {

      XrCompositionLayerProjection *layer = nullptr;
      XrTime frameTime;
      XrView views[2]{
          {XR_TYPE_VIEW},
          {XR_TYPE_VIEW},
      };
      if (xrfwBeginFrame(&frameTime, views)) {
        if (auto projection = oxr.RenderLayer(frameTime, views)) {
          layer = &projection.value();
        }
      }
      xrfwEndFrame(frameTime, layer);

    } else {
      // session is not active
      Sleep(20);
    }

    // glClear(GL_COLOR_BUFFER_BIT);
    // glfwSwapBuffers(window);
  }

  xrfwDestroySession(session);
  xrfwDestroyInstance();
  glfwTerminate();
  return 0;
}
