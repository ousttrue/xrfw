#pragma once
#include <stdint.h>

#ifdef XRFW_BUILD
#define XRFW_API extern "C" __declspec(dllexport)
#else
#define XRFW_API extern "C" __declspec(dllimport)
#endif

XRFW_API int xrfwCreateInstance(const char **extensionNames,
                                uint32_t extensionCount);
XRFW_API void xrfwDestroyInstance();

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
XRFW_API void *xrfwCreateOpenGLWin32Session(HDC hDC, HGLRC hGLRC);

XRFW_API void xrfwDestroySession(void *session);
