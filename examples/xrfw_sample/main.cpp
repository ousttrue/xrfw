#include <windows.h>
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <Glfw/glfw3native.h>

#include <plog/Log.h>
#include <xrfw.h>

#include "oglrenderer.h"

int main(int argc, char **argv) {
  // OpenGL context
  if (!glfwInit()) {
    return 11;
  }

  // Create a windowed mode window and its OpenGL context
  auto window = glfwCreateWindow(640, 480, "Hello xrfw", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 12;
  }
  // Make the window's context current
  glfwMakeContextCurrent(window);
  PLOG_INFO << glGetString(GL_VERSION);

  // require openxr graphics extension
  const char *extensions[] = {
      XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
  };
  auto instance = xrfwCreateInstance(extensions, 1);
  if (!instance) {
    return 1;
  }

  // session from graphics
  auto session = xrfwCreateOpenGLWin32Session(GetDC(glfwGetWin32Window(window)),
                                              glfwGetWGLContext(window));
  if (!session) {
    return 2;
  }

  // swapchain
  XrViewConfigurationView viewConfigurationViews[2];
  if (!xrfwGetViewConfigurationViews(viewConfigurationViews, 2)) {
    return 3;
  }
  int left_width, left_height;
  auto left =
      xrfwCreateSwapchain(viewConfigurationViews[0], &left_width, &left_height);
  if (!left) {
    return 4;
  }
  int right_width, right_height;
  auto right = xrfwCreateSwapchain(viewConfigurationViews[1], &right_width,
                                   &right_height);
  if (!right) {
    return 5;
  }

  OglRenderer renderer;

  // glfw mainloop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if (xrfwPollEventsIsSessionActive()) {

      XrTime frameTime;
      XrfwViewMatrices viewMatrix;
      if (xrfwBeginFrame(&frameTime, &viewMatrix)) {
        // left
        if (auto swapchainImage =
                xrfwAcquireSwapchain(0, left, left_width, left_height)) {
          const uint32_t colorTexture =
              reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(
                  swapchainImage)
                  ->image;
          renderer.Render(colorTexture, left_width, left_height,
                          viewMatrix.leftProjection, viewMatrix.leftView);
          xrfwReleaseSwapchain(left);
        }
        // right
        if (auto swapchainImage =
                xrfwAcquireSwapchain(1, right, right_width, right_height)) {
          const uint32_t colorTexture =
              reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(
                  swapchainImage)
                  ->image;
          renderer.Render(colorTexture, right_width, right_height,
                          viewMatrix.rightProjection, viewMatrix.rightView);
          xrfwReleaseSwapchain(right);
        }
      }
      xrfwEndFrame();
    } else {
      // session is not active
      Sleep(30);
    }

    // glClear(GL_COLOR_BUFFER_BIT);
    // glfwSwapBuffers(window);
  }

  xrfwDestroySession(session);
  xrfwDestroyInstance();
  glfwTerminate();
  return 0;
}
