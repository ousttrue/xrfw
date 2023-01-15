#pragma once
#include "xrfw.h"

XRFW_API XrBool32 xrfwInitializeLoaderAndroid(struct android_app *state);

#if XR_USE_GRAPHICS_API_OPENGL_ES
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <openxr/openxr_platform.h>
XRFW_API void xrfwInitExtensionsAndroidOpenGLES(
    XrGraphicsRequirementsOpenGLESKHR *graphicsRequirements,
    struct android_app *state);
XRFW_API XrSession xrfwCreateSessionAndroidOpenGLES(XrfwSwapchains *swapchains,
                                                    EGLDisplay display,
                                                    EGLContext context);
#endif
