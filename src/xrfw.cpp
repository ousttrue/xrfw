#include <openxr/openxr.h>
#include <plog/Log.h>
#include <xrfw.h>

XrInstance g_instance = nullptr;

XRFW_API int xrfwInit() {

  XrApplicationInfo appInfo{
      .applicationName = "xrfw_sample",
      .applicationVersion = 0,
      .engineName = "xrfw_sample_engine",
      .engineVersion = 0,
      .apiVersion = XR_CURRENT_API_VERSION,
  };

  XrInstanceCreateInfo instanceCreateInfo{
      .type = XR_TYPE_INSTANCE_CREATE_INFO,
      .next = nullptr,
      .createFlags = 0,
      .applicationInfo = appInfo,
      .enabledApiLayerCount = 0,
      .enabledApiLayerNames = NULL,
      .enabledExtensionCount = 0,
      .enabledExtensionNames = nullptr,
  };

  auto result = xrCreateInstance(&instanceCreateInfo, &g_instance);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "Failed to create XR instance:" << result;
    return false;
  }

  return true;
}

XRFW_API void xrfwTerminate() { xrDestroyInstance(g_instance); }
