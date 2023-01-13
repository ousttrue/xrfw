#pragma once
#include "xrfw.h"
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

XRFW_API void xrfwPlatformWin32OpenGL(XrfwInitialization *init);
XRFW_API XrSession xrfwCreateSessionWin32OpenGL(XrfwSwapchains *swapchains,
                                                HDC dc, HGLRC glrc);