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
#include <xrfw.h>

int main(int argc, char **argv) {

  const char *extensions[] = {
      XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
  };

  if (!xrfwCreateInstance(extensions, 1)) {
    return 1;
  }

  // OpenGL context
  if (!glfwInit()) {
    return -1;
  }

  // Create a windowed mode window and its OpenGL context
  auto window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -2;
  }

  // Make the window's context current
  glfwMakeContextCurrent(window);
  PLOG_INFO << glGetString(GL_VERSION);

  auto session = xrfwCreateOpenGLWin32Session(GetDC(glfwGetWin32Window(window)),
                                              glfwGetWGLContext(window));
  if (session) {
    xrfwDestroyInstance();
    return 2;
  }

  // mainloop
  while (false) {
  }

  xrfwDestroySession(session);

  xrfwDestroyInstance();

  return 0;
}
