#ifdef XR_USE_PLATFORM_ANDROID
#include "platform.h"
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include <openxr/openxr_platform.h>
#include <plog/Log.h>

#define OPENGL_VERSION_MAJOR 3
#define OPENGL_VERSION_MINOR 2

struct ksGpuSurfaceBits {
  unsigned char redBits;
  unsigned char greenBits;
  unsigned char blueBits;
  unsigned char alphaBits;
  unsigned char colorBits;
  unsigned char depthBits;
};

enum ksGpuSurfaceColorFormat {
  KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5,
  KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5,
  KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8,
  KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8,
  KS_GPU_SURFACE_COLOR_FORMAT_MAX
};

enum ksGpuSurfaceDepthFormat {
  KS_GPU_SURFACE_DEPTH_FORMAT_NONE,
  KS_GPU_SURFACE_DEPTH_FORMAT_D16,
  KS_GPU_SURFACE_DEPTH_FORMAT_D24,
  KS_GPU_SURFACE_DEPTH_FORMAT_MAX
};

enum ksGpuSampleCount {
  KS_GPU_SAMPLE_COUNT_1 = 1,
  KS_GPU_SAMPLE_COUNT_2 = 2,
  KS_GPU_SAMPLE_COUNT_4 = 4,
  KS_GPU_SAMPLE_COUNT_8 = 8,
  KS_GPU_SAMPLE_COUNT_16 = 16,
  KS_GPU_SAMPLE_COUNT_32 = 32,
  KS_GPU_SAMPLE_COUNT_64 = 64,
};

struct ksGpuLimits {
  size_t maxPushConstantsSize;
  int maxSamples;
};

struct ksGpuContext {
  // const ksGpuDevice *device;
  EGLDisplay display;
  EGLConfig config;
  EGLSurface tinySurface;
  EGLSurface mainSurface;
  EGLContext context;
};

static const char *EglErrorString(const EGLint error) {
  switch (error) {
  case EGL_SUCCESS:
    return "EGL_SUCCESS";
  case EGL_NOT_INITIALIZED:
    return "EGL_NOT_INITIALIZED";
  case EGL_BAD_ACCESS:
    return "EGL_BAD_ACCESS";
  case EGL_BAD_ALLOC:
    return "EGL_BAD_ALLOC";
  case EGL_BAD_ATTRIBUTE:
    return "EGL_BAD_ATTRIBUTE";
  case EGL_BAD_CONTEXT:
    return "EGL_BAD_CONTEXT";
  case EGL_BAD_CONFIG:
    return "EGL_BAD_CONFIG";
  case EGL_BAD_CURRENT_SURFACE:
    return "EGL_BAD_CURRENT_SURFACE";
  case EGL_BAD_DISPLAY:
    return "EGL_BAD_DISPLAY";
  case EGL_BAD_SURFACE:
    return "EGL_BAD_SURFACE";
  case EGL_BAD_MATCH:
    return "EGL_BAD_MATCH";
  case EGL_BAD_PARAMETER:
    return "EGL_BAD_PARAMETER";
  case EGL_BAD_NATIVE_PIXMAP:
    return "EGL_BAD_NATIVE_PIXMAP";
  case EGL_BAD_NATIVE_WINDOW:
    return "EGL_BAD_NATIVE_WINDOW";
  case EGL_CONTEXT_LOST:
    return "EGL_CONTEXT_LOST";
  default:
    return "unknown";
  }
}

static ksGpuSurfaceBits
ksGpuContext_BitsForSurfaceFormat(const ksGpuSurfaceColorFormat colorFormat,
                                  const ksGpuSurfaceDepthFormat depthFormat) {
  ksGpuSurfaceBits bits;
  bits.redBits =
      ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
           ? 8
           : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                  ? 8
                  : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                         ? 5
                         : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5)
                                ? 5
                                : 8))));
  bits.greenBits =
      ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
           ? 8
           : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                  ? 8
                  : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                         ? 6
                         : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5)
                                ? 6
                                : 8))));
  bits.blueBits =
      ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
           ? 8
           : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                  ? 8
                  : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                         ? 5
                         : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5)
                                ? 5
                                : 8))));
  bits.alphaBits =
      ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
           ? 8
           : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                  ? 8
                  : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                         ? 0
                         : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5)
                                ? 0
                                : 8))));
  bits.colorBits =
      bits.redBits + bits.greenBits + bits.blueBits + bits.alphaBits;
  bits.depthBits =
      ((depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D16)
           ? 16
           : ((depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D24) ? 24 : 0));
  return bits;
}

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

  bool InitializeGraphics() {
    //
    // Initialize the loader for this platform
    //
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

    //
    // EGL
    //
    graphicsBinding_.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(graphicsBinding_.display, &majorVersion_, &minorVersion_);
    PLOG_INFO << "eglInitialize => " << majorVersion_ << "." << minorVersion_;

    // EGLint numConfigs;
    // /*
    //  * Here specify the attributes of the desired configuration.
    //  * Below, we select an EGLConfig with at least 8 bits per color
    //  * component compatible with on-screen windows
    //  */
    // const EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    //                           EGL_NONE};
    // EGLConfig config;
    // eglChooseConfig(graphicsBinding_.display, attribs, &config, 1,
    // &numConfigs);

    // EGLint format;
    // /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
    //  * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
    //  * As soon as we picked a EGLConfig, we can safely reconfigure the
    //  * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    // eglGetConfigAttrib(graphicsBinding_.display, config,
    // EGL_NATIVE_VISUAL_ID,
    //                    &format);
    // surface_ = eglCreateWindowSurface(graphicsBinding_.display, config,
    //                                   app_->window, nullptr);
    // if (!surface_) {
    //   PLOG_FATAL << "eglCreateWindowSurface";
    //   return false;
    // }

    // EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    // graphicsBinding_.context = eglCreateContext(
    //     graphicsBinding_.display, config, EGL_NO_CONTEXT, contextAttribs);
    // if (graphicsBinding_.context == EGL_NO_CONTEXT) {
    //   PLOG_FATAL << "eglCreateContext failed with error 0x" << std::hex
    //              << eglGetError();
    //   return false;
    // }

    // if (eglMakeCurrent(graphicsBinding_.display, surface_, surface_,
    //                    graphicsBinding_.context) == EGL_FALSE) {
    //   PLOG_FATAL << "Unable to eglMakeCurrent";
    //   return false;
    // }

    // Do NOT use eglChooseConfig, because the Android EGL code pushes in
    // multisample flags in eglChooseConfig when the user has selected the
    // "force 4x MSAA" option in settings, and that is completely wasted on the
    // time warped frontbuffer.
    enum { MAX_CONFIGS = 1024 };
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;
    if (!eglGetConfigs(graphicsBinding_.display, configs, MAX_CONFIGS,
                       &numConfigs)) {
      PLOG_FATAL << "eglGetConfigs";
      return false;
    }

    ksGpuSurfaceColorFormat colorFormat{KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8};
    ksGpuSurfaceDepthFormat depthFormat{KS_GPU_SURFACE_DEPTH_FORMAT_D24};
    ksGpuSampleCount sampleCount{KS_GPU_SAMPLE_COUNT_1};
    const ksGpuSurfaceBits bits =
        ksGpuContext_BitsForSurfaceFormat(colorFormat, depthFormat);

    const EGLint configAttribs[] = {
        EGL_RED_SIZE, bits.greenBits, EGL_GREEN_SIZE, bits.redBits,
        EGL_BLUE_SIZE, bits.blueBits, EGL_ALPHA_SIZE, bits.alphaBits,
        EGL_DEPTH_SIZE, bits.depthBits,
        // EGL_STENCIL_SIZE,	0,
        EGL_SAMPLE_BUFFERS, (sampleCount > KS_GPU_SAMPLE_COUNT_1), EGL_SAMPLES,
        (sampleCount > KS_GPU_SAMPLE_COUNT_1) ? sampleCount : 0, EGL_NONE};

    // context->config = 0;
    EGLConfig config = 0;
    for (int i = 0; i < numConfigs; i++) {
      EGLint value = 0;

      eglGetConfigAttrib(graphicsBinding_.display, configs[i],
                         EGL_RENDERABLE_TYPE, &value);
      if ((value & EGL_OPENGL_ES3_BIT) != EGL_OPENGL_ES3_BIT) {
        continue;
      }

      // Without EGL_KHR_surfaceless_context, the config needs to support both
      // pbuffers and window surfaces.
      eglGetConfigAttrib(graphicsBinding_.display, configs[i], EGL_SURFACE_TYPE,
                         &value);
      if ((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) !=
          (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) {
        continue;
      }

      int j = 0;
      for (; configAttribs[j] != EGL_NONE; j += 2) {
        eglGetConfigAttrib(graphicsBinding_.display, configs[i],
                           configAttribs[j], &value);
        if (value != configAttribs[j + 1]) {
          break;
        }
      }
      if (configAttribs[j] == EGL_NONE) {
        config = configs[i];
        break;
      }
    }
    if (config == 0) {
      PLOG_FATAL << "Failed to find EGLConfig";
      return false;
    }

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, OPENGL_VERSION_MAJOR,
                               EGL_NONE, EGL_NONE, EGL_NONE};
    // Use the default priority if KS_GPU_QUEUE_PRIORITY_MEDIUM is selected.
    // const ksGpuQueuePriority priority =
    //     device->queueInfo.queuePriorities[queueIndex];
    // if (priority != KS_GPU_QUEUE_PRIORITY_MEDIUM) {
    //   contextAttribs[2] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
    //   contextAttribs[3] = (priority == KS_GPU_QUEUE_PRIORITY_LOW)
    //                           ? EGL_CONTEXT_PRIORITY_LOW_IMG
    //                           : EGL_CONTEXT_PRIORITY_HIGH_IMG;
    // }
    graphicsBinding_.context = eglCreateContext(
        graphicsBinding_.display, config, EGL_NO_CONTEXT, contextAttribs);
    if (graphicsBinding_.context == EGL_NO_CONTEXT) {
      PLOG_FATAL << "eglCreateContext() failed: "
                 << EglErrorString(eglGetError());
      return false;
    }

    const EGLint surfaceAttribs[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
    auto tinySurface = eglCreatePbufferSurface(graphicsBinding_.display, config,
                                               surfaceAttribs);
    if (tinySurface == EGL_NO_SURFACE) {
      PLOG_FATAL << "eglCreatePbufferSurface() failed: "
                 << EglErrorString(eglGetError());
      eglDestroyContext(graphicsBinding_.display, graphicsBinding_.context);
      graphicsBinding_.context = EGL_NO_CONTEXT;
      return false;
    }
    surface_ = tinySurface;

    if (!eglMakeCurrent(graphicsBinding_.display, surface_, surface_,
                        graphicsBinding_.context)) {
      return false;
    }

    return true;
  }

  bool BeginFrame() {
    if (exitRenderLoop) {
      return false;
    }

    // Read all pending android events.
    for (;;) {
      int timeout = -1; // blocking
      if (Resumed || requestRestart) {
        timeout = 0; // non blocking
      }

      int events;
      struct android_poll_source *source;
      if (ALooper_pollAll(timeout, nullptr, &events, (void **)&source) < 0) {
        break;
      }

      if (source) {
        source->process(app_, source);
      }
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
  return &impl_->instanceCreateInfoAndroid_;
}
bool Platform::InitializeGraphics() { return impl_->InitializeGraphics(); }
bool Platform::BeginFrame() { return impl_->BeginFrame(); }

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