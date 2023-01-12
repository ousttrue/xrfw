#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
// #include <android_native_app_glue.h>

struct Platform {
  // require openxr graphics extension
  const char *extensions[1] = {
      XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
  };

  Platform() {}
  ~Platform() {}
  bool InitializeGraphics() { return false; }
};
#endif
