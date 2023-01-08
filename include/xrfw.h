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
inline std::ostream &operator<<(std::ostream &os, XrResult value) {
  char buffer[XR_MAX_RESULT_STRING_SIZE];
  if (XR_SUCCEEDED(xrResultToString(xrfwGetInstance(), value, buffer))) {
    os << std::string_view(buffer);
  } else {
    os << value;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os, XrStructureType value) {
  char buffer[XR_MAX_STRUCTURE_NAME_SIZE];
  if (XR_SUCCEEDED(xrStructureTypeToString(xrfwGetInstance(), value, buffer))) {
    os << std::string_view(buffer);
  } else {
    os << value;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os, XrSessionState value) {
  switch (value) {
  case XR_SESSION_STATE_UNKNOWN:
    os << std::string_view("XR_SESSION_STATE_UNKNOWN");
    break;
  case XR_SESSION_STATE_IDLE:
    os << std::string_view("XR_SESSION_STATE_IDLE");
    break;
  case XR_SESSION_STATE_READY:
    os << std::string_view("XR_SESSION_STATE_READY");
    break;
  case XR_SESSION_STATE_SYNCHRONIZED:
    os << std::string_view("XR_SESSION_STATE_SYNCHRONIZED");
    break;
  case XR_SESSION_STATE_VISIBLE:
    os << std::string_view("XR_SESSION_STATE_VISIBLE");
    break;
  case XR_SESSION_STATE_FOCUSED:
    os << std::string_view("XR_SESSION_STATE_FOCUSED");
    break;
  case XR_SESSION_STATE_STOPPING:
    os << std::string_view("XR_SESSION_STATE_STOPPING");
    break;
  case XR_SESSION_STATE_LOSS_PENDING:
    os << std::string_view("XR_SESSION_STATE_LOSS_PENDING");
    break;
  case XR_SESSION_STATE_EXITING:
    os << std::string_view("XR_SESSION_STATE_EXITING");
    break;
  default:
    os << value;
    break;
  }
  return os;
}
