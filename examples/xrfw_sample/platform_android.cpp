#ifdef XR_USE_PLATFORM_ANDROID
#include "platform.h"
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES3/gl32.h>

#include <openxr/openxr_platform.h>
#include <plog/Log.h>

struct PlatformImpl {
  JNIEnv *Env;
  ANativeWindow *NativeWindow = nullptr;
  bool Resumed = false;
  // void *applicationVM = nullptr;
  // void *applicationActivity = nullptr;
  struct android_app *app_ = nullptr;
  bool requestRestart = false;
  bool exitRenderLoop = false;
  // EGL
  EGLint majorVersion_;
  EGLint minorVersion_;
  EGLSurface surface_ = 0;
  EGLint width_ = 0;
  EGLint height_ = 0;
  XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding_ = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
  };

  XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid_ = {};
  // require openxr graphics extension
  const char *extensions[2] = {
      XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
      XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
  };

  PlatformImpl(struct android_app *state) {
    state->activity->vm->AttachCurrentThread(&Env, nullptr);

    state->userData = this;
    state->onAppCmd = app_handle_cmd;
    app_ = state;
    instanceCreateInfoAndroid_ = {
        .type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
        .applicationVM = app_->activity->vm,
        .applicationActivity = app_->activity->clazz,
    };
  }
  ~PlatformImpl() {}

  bool Initialize() {

    graphicsBinding_.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(graphicsBinding_.display, &majorVersion_, &minorVersion_);

    EGLint numConfigs;
    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                              EGL_NONE};
    EGLConfig config;
    eglChooseConfig(graphicsBinding_.display, attribs, &config, 1, &numConfigs);

    EGLint format;
    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(graphicsBinding_.display, config, EGL_NATIVE_VISUAL_ID,
                       &format);
    surface_ = eglCreateWindowSurface(graphicsBinding_.display, config,
                                      app_->window, nullptr);

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    graphicsBinding_.context = eglCreateContext(
        graphicsBinding_.display, config, EGL_NO_CONTEXT, contextAttribs);
    if (graphicsBinding_.context == EGL_NO_CONTEXT) {
      PLOG_FATAL << "eglCreateContext failed with error 0x" << std::hex
                 << eglGetError();
      return false;
    }

    if (eglMakeCurrent(graphicsBinding_.display, surface_, surface_,
                       graphicsBinding_.context) == EGL_FALSE) {
      PLOG_FATAL << "Unable to eglMakeCurrent";
      return false;
    }

    return true;
  }
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
        .applicationVM = app_->activity->vm,
        .applicationContext = app_->activity->clazz,
    };
    if (XR_FAILED(initializeLoader(
            (const XrLoaderInitInfoBaseHeaderKHR *)&loaderInitInfoAndroid))) {
      PLOG_FATAL << "xrInitializeLoaderKHR";
      return false;
    }

    if (!Initialize()) {
      return false;
    }

    // Initialize the gl extensions. Note we have to open a window.
    // ksDriverInstance driverInstance{};
    // ksGpuQueueInfo queueInfo{};
    // ksGpuSurfaceColorFormat
    // colorFormat{KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8};
    // ksGpuSurfaceDepthFormat depthFormat{KS_GPU_SURFACE_DEPTH_FORMAT_D24};
    // ksGpuSampleCount sampleCount{KS_GPU_SAMPLE_COUNT_1};
    // if (!ksGpuWindow_Create(&window, &driverInstance, &queueInfo, 0,
    // colorFormat,
    //                         depthFormat, sampleCount, 640, 480, false)) {
    //   THROW("Unable to create GL context");
    // }

    // GLint major = 0;
    // GLint minor = 0;
    // glGetIntegerv(GL_MAJOR_VERSION, &major);
    // glGetIntegerv(GL_MINOR_VERSION, &minor);
    // const XrVersion desiredApiVersion = XR_MAKE_VERSION(major, minor, 0);
    // if (graphicsRequirements.minApiVersionSupported > desiredApiVersion) {
    //   THROW("Runtime does not support desired Graphics API and/or version");
    // }

    // m_contextApiMajorVersion = major;

    // #if defined(XR_USE_PLATFORM_ANDROID)
    //       m_graphicsBinding.display = window.display;
    //     m_graphicsBinding.config = (EGLConfig)0;
    //     m_graphicsBinding.context = window.context.context;
    // #endif

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
  return &impl_->instanceCreateInfoAndroid_;
}
bool Platform::InitializeGraphics() { return impl_->InitializeGraphics(); }
bool Platform::BeginFrame() {
  if (impl_->exitRenderLoop) {
    return false;
  }
  // process_android_event(app, appState, xr);
  // Read all pending android events.
  for (;;) {

    int timeout = -1; // blocking
    if (impl_->Resumed || impl_->requestRestart) {
      timeout = 0; // non blocking
    }

    int events;
    struct android_poll_source *source;
    if (ALooper_pollAll(timeout, nullptr, &events, (void **)&source) < 0) {
      break;
    }

    if (source != nullptr) {
      source->process(impl_->app_, source);
    }
  }

  return true;
}

void Platform::EndFrame(OglRenderer &renderer) {}
uint32_t
Platform::CastTexture(const XrSwapchainImageBaseHeader *swapchainImage) {
  return reinterpret_cast<const XrSwapchainImageOpenGLESKHR *>(swapchainImage)
      ->image;
}
void Platform::Sleep(std::chrono::milliseconds ms) {}
std::span<const char *> Platform::Extensions() const {
  return impl_->extensions;
}
const void *Platform::GraphicsBinding() const {
  return &impl_->graphicsBinding_;
}
#endif