#pragma once
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#ifdef XRFW_BUILD
#define XRFW_API extern "C" __declspec(dllexport)
#else
#define XRFW_API extern "C" __declspec(dllimport)
#endif

XRFW_API XrSession xrfwCreateOpenGLWin32SessionAndSwapchain(XrfwSwapchains*views, HDC hDC, HGLRC hGLRC);