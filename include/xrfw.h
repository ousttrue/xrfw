#pragma once

#ifdef XRFW_BUILD
#define XRFW_API extern "C" __declspec(dllexport)
#else
#define XRFW_API extern "C" __declspec(dllimport)
#endif

XRFW_API int xrfwInit();
XRFW_API void xrfwTerminate();
