#pragma once
#include "xrfw.h"

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>
#include <android/sensor.h>

XRFW_API XrBool32 xrfwInitializeLoaderAndroid(struct android_app *state);
XRFW_API void xrfwPlatformAndroidOpenGLES(XrfwInitialization *init,
                                          struct android_app *state);
XRFW_API XrSession xrfwCreateSessionAndroidOpenGLES(XrfwSwapchains *swapchains,
                                                    EGLDisplay display,
                                                    EGLContext context);
