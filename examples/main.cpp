#include <windows.h>
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

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

  auto session = xrfwCreateOpenGLWin32Session(nullptr, nullptr);
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
