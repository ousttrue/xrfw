#pragma once
#include <algorithm>
#include <openxr/openxr.h>
#include <plog/Log.h>

template<size_t N>
struct XrFwStringLiteral
{
  constexpr XrFwStringLiteral(const char (&str)[N])
  {
    std::copy_n(str, N, value);
  }
  char value[N];
};

template<typename PFN, XrFwStringLiteral NAME>
struct XrFwInstanceProc
{
  PFN Proc = nullptr;
  XrFwInstanceProc(XrInstance instance)
  {
    if (XR_FAILED(xrGetInstanceProcAddr(
          instance, NAME.value, (PFN_xrVoidFunction*)&Proc))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: " << NAME.value;
    }
  }
  template<typename... ARGS>
  auto operator()(ARGS... args) const
  {
    return Proc(args...);
  }
};

#define XRFW_PROC(name) XrFwInstanceProc<PFN_##name, #name> name;
