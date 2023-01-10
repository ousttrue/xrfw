#include "glm/gtx/quaternion.hpp"
#include "ogldrawable.h"
#include "openxr/openxr.h"
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
  OglInitialize();

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
  auto drawable = OglDrawable::Create();

  // glfw mainloop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if (xrfwPollEventsIsSessionActive()) {

      // For each locatable space that we want to visualize, render a 25cm cube.
      std::vector<Cube> cubes;
      cubes.push_back(Cube{
          {0, 0, 0, 1},
          {0, 0, 0},
          {0.25f, 0.25f, 0.25f},
      });

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
          // render
          renderer.BeginFbo(colorTexture, left_width, left_height);
          // auto projection = projection_matrix(views[0].fov);
          // auto view = view_matrix(views[0].pose);
          drawable->Render(viewMatrix.leftProjection, viewMatrix.leftView, cubes);
          renderer.EndFbo();
          xrfwReleaseSwapchain(left);
        }
        // right
        if (auto swapchainImage =
                xrfwAcquireSwapchain(1, right, right_width, right_height)) {
          const uint32_t colorTexture =
              reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(
                  swapchainImage)
                  ->image;
          // render
          renderer.BeginFbo(colorTexture, right_width, right_height);
          drawable->Render(viewMatrix.rightProjection, viewMatrix.rightView, cubes);
          renderer.EndFbo();
          xrfwReleaseSwapchain(right);
        }
      }
      xrfwEndFrame();
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
