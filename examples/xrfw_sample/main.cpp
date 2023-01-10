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

  // session and swapchains from graphics
  XrfwSwapchains swapchains;
  auto session = xrfwCreateOpenGLWin32SessionAndSwapchain(
      &swapchains, GetDC(glfwGetWin32Window(window)),
      glfwGetWGLContext(window));
  if (!session) {
    return 2;
  }

  OglRenderer renderer;

  // glfw mainloop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // OpenXR handling
    if (xrfwPollEventsIsSessionActive()) {
      XrTime frameTime;
      XrfwViewMatrices viewMatrix;
      if (xrfwBeginFrame(&frameTime, &viewMatrix)) {
        // left
        if (auto swapchainImage = xrfwAcquireSwapchain(swapchains.left)) {
          const uint32_t colorTexture =
              reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(
                  swapchainImage)
                  ->image;
          renderer.Render(colorTexture, swapchains.leftWidth,
                          swapchains.leftHeight, viewMatrix.leftProjection,
                          viewMatrix.leftView);
          xrfwReleaseSwapchain(swapchains.left);
        }
        // right
        if (auto swapchainImage = xrfwAcquireSwapchain(swapchains.right)) {
          const uint32_t colorTexture =
              reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(
                  swapchainImage)
                  ->image;
          renderer.Render(colorTexture, swapchains.rightWidth,
                          swapchains.rightHeight, viewMatrix.rightProjection,
                          viewMatrix.rightView);
          xrfwReleaseSwapchain(swapchains.right);
        }
      }
      xrfwEndFrame();
    } else {
      // XrSession is not active
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
