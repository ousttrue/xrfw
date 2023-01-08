#include "openxr/openxr.h"
#include <windows.h>
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <Glfw/glfw3native.h>

#include <plog/Log.h>

#include "oxrapp.h"
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
  OxrApp app(instance, session);
  if (!app.Initialize()) {
    return 3;
  }

  // glfw mainloop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    // glClear(GL_COLOR_BUFFER_BIT);
    // glfwSwapBuffers(window);
    if (!app.ProcessFrame()) {
      break;
    }
  }

  xrfwDestroySession(session);
  xrfwDestroyInstance();
  glfwTerminate();
  return 0;
}
