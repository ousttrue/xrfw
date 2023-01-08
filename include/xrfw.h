#pragma once
#include <string_view>
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#include <openxr/openxr.h>
#include <stdint.h>

#ifdef XRFW_BUILD
#define XRFW_API extern "C" __declspec(dllexport)
#else
#define XRFW_API extern "C" __declspec(dllimport)
#endif

XRFW_API XrInstance xrfwCreateInstance(const char **extensionNames,
                                       uint32_t extensionCount);
XRFW_API void xrfwDestroyInstance();

XRFW_API XrInstance xrfwGetInstance();

XRFW_API XrSession xrfwCreateOpenGLWin32Session(HDC hDC, HGLRC hGLRC);

XRFW_API void xrfwDestroySession(void *session);

#include <iosfwd>
inline std::ostream &operator<<(std::ostream &os, XrResult result) {
  char buffer[XR_MAX_RESULT_STRING_SIZE];
  if (XR_SUCCEEDED(xrResultToString(xrfwGetInstance(), result, buffer))) {
    os << std::string_view(buffer);
  } else {
    os << result;
  }
  return os;
}
