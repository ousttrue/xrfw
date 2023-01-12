#include <plog/Log.h>
#include <xrfw.h>

#include "oglrenderer.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <numbers>

#ifdef XR_USE_PLATFORM_WIN32
//
// windows
//
#include <openxr/openxr_platform.h>
#include <windows.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <Glfw/glfw3native.h>

struct TurnTable {
  glm::vec3 shift = {0, 0, -5};
  float yaw = 0;
  float pitch = 0;
  float fovY = static_cast<float>(30.0 / 180 * std::numbers::pi);
  float zNear = 0.01f;
  float zFar = 100.0f;
  int width = 512;
  int height = 512;
  float projection[16];
  float view[16];
  bool isRightDown = false;
  int mouse_x = 0;
  int mouse_y = 0;

  void update() {
    *((glm::mat4 *)projection) =
        glm::perspectiveFovRH_ZO(fovY, static_cast<float>(width),
                                 static_cast<float>(height), zNear, zFar);

    auto t = glm::translate(glm::mat4(1), shift);
    auto r = glm::angleAxis(yaw, glm::vec3{0, 1, 0}) *
             glm::angleAxis(pitch, glm::vec3{1, 0, 0});
    *((glm::mat4 *)view) = t * glm::toMat4(r);
  }

  void rightMousePress() { isRightDown = true; }
  void rightMouseRelease() { isRightDown = false; }
  void cursor(int x, int y) {
    auto dx = x - mouse_x;
    auto dy = y - mouse_y;
    mouse_x = x;
    mouse_y = y;

    if (isRightDown) {
      yaw += dx * 0.01f;
      pitch += dy * 0.01f;
      update();
    }
  }
};

static void cursor_position_callback(GLFWwindow *window, double xpos,
                                     double ypos) {
  auto camera = (TurnTable *)glfwGetWindowUserPointer(window);
  camera->cursor(xpos, ypos);
}
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  auto camera = (TurnTable *)glfwGetWindowUserPointer(window);
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    switch (action) {
    case GLFW_PRESS:
      camera->rightMousePress();
      break;

    case GLFW_RELEASE:
      camera->rightMouseRelease();
      break;
    }
  }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  auto camera = (TurnTable *)glfwGetWindowUserPointer(window);
  camera->width = width;
  camera->height = height;
  camera->update();
}

struct Platform {
  GLFWwindow *window_ = nullptr;
  TurnTable camera;
  // require openxr graphics extension
  const char *extensions[1] = {
      XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
  };
  XrGraphicsBindingOpenGLWin32KHR graphicsBindingGL_ = {};

  Platform() {
    if (!glfwInit()) {
      PLOG_FATAL << "glfwInit";
      throw std::runtime_error("glfwInit");
    }
  }
  ~Platform() { glfwTerminate(); }
  bool InitializeGraphics() {
    // Create a windowed mode window and its OpenGL context
    window_ = glfwCreateWindow(640, 480, "Hello xrfw", nullptr, nullptr);
    if (!window_) {
      return false;
    }
    // Make the window's context current
    glfwMakeContextCurrent(window_);
    PLOG_INFO << glGetString(GL_VERSION);

    glfwGetFramebufferSize(window_, &camera.width, &camera.height);
    camera.update();
    glfwSetWindowUserPointer(window_, &camera);
    glfwSetCursorPosCallback(window_, cursor_position_callback);
    glfwSetMouseButtonCallback(window_, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);

    graphicsBindingGL_ = {
        .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
        .next = nullptr,
        .hDC = GetDC(glfwGetWin32Window(window_)),
        .hGLRC = glfwGetWGLContext(window_),
    };

    return true;
  }

  bool BeginFrame() {
    if (glfwWindowShouldClose(window_)) {
      return false;
    }
    glfwPollEvents();
    return true;
  }

  void EndFrame(OglRenderer &renderer) {
    // glClear(GL_COLOR_BUFFER_BIT);
    renderer.Render(0, camera.width, camera.height, camera.projection,
                    camera.view);
    glfwSwapBuffers(window_);
  }
};
#elif XR_USE_PLATFORM_ANDROID
//
// Android
//
#endif

int start() {
  Platform platform;
  if (!platform.InitializeGraphics()) {
    return 1;
  }

  // instance
  auto instance = xrfwCreateInstance(platform.extensions, 1);
  if (!instance) {
    return 2;
  }

  // session and swapchains from graphics
  XrfwSwapchains swapchains;
  auto session = xrfwCreateSession(&swapchains, &platform.graphicsBindingGL_);
  if (!session) {
    return 3;
  }

  OglRenderer renderer;

  // glfw mainloop
  while (platform.BeginFrame()) {

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

    platform.EndFrame(renderer);
  }

  xrfwDestroySession(session);
  xrfwDestroyInstance();
  return 0;
}

int main(int argc, char **argv) { return start(); }
