#ifdef XR_USE_PLATFORM_ANDROID
#include "platform.h"
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <openxr/openxr_platform.h>
#include <plog/Log.h>

struct PlatformImpl {
  ANativeWindow *NativeWindow = nullptr;
  bool Resumed = false;
  void *applicationVM = nullptr;
  void *applicationActivity = nullptr;
  bool requestRestart = false;
  bool exitRenderLoop = false;

  XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid;
  // require openxr graphics extension
  const char *extensions[1] = {
      XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
  };

  PlatformImpl(struct android_app *state) {
    state->userData = this;
    state->onAppCmd = app_handle_cmd;
    applicationVM = state->activity->vm;
    applicationActivity = state->activity->clazz;
    instanceCreateInfoAndroid = {
        .type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
        .applicationVM = applicationVM,
        .applicationActivity = applicationActivity,
    };
  }
  ~PlatformImpl() {}
  bool InitializeGraphics() {
    // Initialize the loader for this platform
    PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
    if (XR_FAILED(
            xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                  (PFN_xrVoidFunction *)(&initializeLoader)))) {
      PLOG_FATAL << "no xrInitializeLoaderKHR";
      return false;
    }

    XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid{
        .type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
        .next = NULL,
        .applicationVM = applicationVM,
        .applicationContext = applicationActivity,
    };
    if (XR_FAILED(initializeLoader(
            (const XrLoaderInitInfoBaseHeaderKHR *)&loaderInitInfoAndroid))) {
      PLOG_FATAL << "xrInitializeLoaderKHR";
      return false;
    }

    return true;
  }

  void handle_android_cmd(struct android_app *app, int32_t cmd) {
    switch (cmd) {
    // There is no APP_CMD_CREATE. The ANativeActivity creates the
    // application thread from onCreate(). The application thread
    // then calls android_main().
    case APP_CMD_START: {
      PLOG_INFO << "    APP_CMD_START";
      PLOG_INFO << "onStart()";
      break;
    }
    case APP_CMD_RESUME: {
      PLOG_INFO << "onResume()";
      PLOG_INFO << "    APP_CMD_RESUME";
      Resumed = true;
      break;
    }
    case APP_CMD_PAUSE: {
      PLOG_INFO << "onPause()";
      PLOG_INFO << "    APP_CMD_PAUSE";
      Resumed = false;
      break;
    }
    case APP_CMD_STOP: {
      PLOG_INFO << "onStop()";
      PLOG_INFO << "    APP_CMD_STOP";
      break;
    }
    case APP_CMD_DESTROY: {
      PLOG_INFO << "onDestroy()";
      PLOG_INFO << "    APP_CMD_DESTROY";
      NativeWindow = NULL;
      break;
    }
    case APP_CMD_INIT_WINDOW: {
      PLOG_INFO << "surfaceCreated()";
      PLOG_INFO << "    APP_CMD_INIT_WINDOW";
      NativeWindow = app->window;
      break;
    }
    case APP_CMD_TERM_WINDOW: {
      PLOG_INFO << "surfaceDestroyed()";
      PLOG_INFO << "    APP_CMD_TERM_WINDOW";
      NativeWindow = NULL;
      break;
    }
    }
  }

  static void app_handle_cmd(struct android_app *app, int32_t cmd) {
    auto impl = (PlatformImpl *)app->userData;
    impl->handle_android_cmd(app, cmd);
  }
};

Platform::Platform(struct android_app *state)
    : impl_(new PlatformImpl(state)) {}
Platform::~Platform() { delete impl_; }
const void *Platform::InstanceNext() const {
  return &impl_->instanceCreateInfoAndroid;
}
bool Platform::InitializeGraphics() { return impl_->InitializeGraphics(); }
bool Platform::BeginFrame() { return false; }
void Platform::EndFrame(OglRenderer &renderer) {}
uint32_t
Platform::CastTexture(const XrSwapchainImageBaseHeader *swapchainImage) {
  return 0;
}
void Platform::Sleep(std::chrono::milliseconds ms) {}
const char *const *Platform::Extensions() const {}
const void *Platform::GraphicsBinding() const {}
#endif