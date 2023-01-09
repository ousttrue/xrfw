#include <gl/glew.h>

#include "oglrenderer.h"
#include <windows.h>
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <Glfw/glfw3native.h>

#include <plog/Log.h>
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
  // Make the window's context current
  glfwMakeContextCurrent(window);
  PLOG_INFO << glGetString(GL_VERSION);
  glewInit();

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

    if (xrfwPollEventsAndIsActive()) {

      XrCompositionLayerProjection *layer = nullptr;
      XrTime frameTime;
      XrView views[2]{
          {XR_TYPE_VIEW},
          {XR_TYPE_VIEW},
      };
      XrCompositionLayerProjectionView projectionLayerViews[2] = {};
      XrCompositionLayerProjection projection{
          .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
          .next = nullptr,
          .layerFlags = 0,
          .space = {},
          .viewCount = 2,
          .views = projectionLayerViews,
      };
      if (xrfwBeginFrame(&frameTime, views)) {
        projectionLayerViews[0] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .pose = views[0].pose,
            .fov = views[0].fov,
            .subImage =
                {
                    .swapchain = left,
                    .imageRect =
                        {
                            .offset = {0, 0},
                            .extent = {left_width, left_height},
                        },
                },
        };
        projectionLayerViews[1] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .pose = views[1].pose,
            .fov = views[1].fov,
            .subImage =
                {
                    .swapchain = right,
                    .imageRect =
                        {
                            .offset = {0, 0},
                            .extent = {right_width, right_height},
                        },
                },
        };
        // left
        if (auto swapchainImage = xrfwAcquireSwapchain(left)) {
          const uint32_t colorTexture =
              reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(
                  swapchainImage)
                  ->image;
          // render
          renderer.RenderView(colorTexture, left_width, left_height, frameTime,
                              views[0]);
          xrfwReleaseSwapchain(left);
        }
        // right
        if (auto swapchainImage = xrfwAcquireSwapchain(right)) {
          const uint32_t colorTexture =
              reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(
                  swapchainImage)
                  ->image;
          // render
          renderer.RenderView(colorTexture, right_width, right_height,
                              frameTime, views[1]);
          xrfwReleaseSwapchain(right);
        }

        layer = &projection;
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
