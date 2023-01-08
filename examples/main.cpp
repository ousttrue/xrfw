#include "openxr/openxr.h"
#include <windows.h>
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <Glfw/glfw3native.h>

#define PLOG_IMPORT
#include <plog/Log.h>

#include <gl/GL.h>
#include <iomanip>
#include <thread>
#include <xrfw.h>

static void xrMainloop(XrSession session, bool *isClosed) {

  XrFrameWaitInfo waitFrameInfo = {
      .type = XR_TYPE_FRAME_WAIT_INFO,
      .next = nullptr,
  };

  XrFrameState frameState = {
      .type = XR_TYPE_FRAME_STATE,
      .next = nullptr,
  };

  while (!*isClosed) {
    auto result = xrWaitFrame(session, &waitFrameInfo, &frameState);
    if (XR_FAILED(result)) {
      return;
    }
  }
}

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
  if (!xrfwCreateInstance(extensions, 1)) {
    return 1;
  }

  // Make the window's context current
  glfwMakeContextCurrent(window);
  PLOG_INFO << glGetString(GL_VERSION);

  auto session = xrfwCreateOpenGLWin32Session(GetDC(glfwGetWin32Window(window)),
                                              glfwGetWGLContext(window));
  if (session) {
    // separate XR mainloop
    bool is_closed = false;
    std::thread xrThread(xrMainloop, session, &is_closed);

    // this is glfw mainloop
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      glClear(GL_COLOR_BUFFER_BIT);
      glfwSwapBuffers(window);
    }

    // cleanup
    is_closed = true;
    xrThread.join();
    xrfwDestroySession(session);
  }

  xrfwDestroyInstance();
  glfwTerminate();
  return 0;
}
