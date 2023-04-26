#pragma once
#include <openxr/openxr.h>
#include <span>
#include <vector>

using XrfwGraphicsRequirementsFunc = bool (*)(XrInstance instance,
                                              XrSystemId systemId,
                                              void *outGraphicsRequirements);
// using XrfwGetSwapchainFormatsFunc = std::vector<int64_t> (*)(XrSession
// session);
using XrfwSelectColorSwapchainFormatFunc =
    int64_t (*)(std::span<int64_t> swapchainFormats);
using XrfwAllocateSwapchainImageStructsFunc =
    std::vector<XrSwapchainImageBaseHeader *> (*)(
        uint32_t capacity, const XrSwapchainCreateInfo &);
struct XrfwInitialization {
  XrfwGraphicsRequirementsFunc graphicsRequirementsCallback = nullptr;
  void *graphicsRequirements = nullptr;
  // XrfwGetSwapchainFormatsFunc getSwapchainFormatsCallback = nullptr;
  XrfwSelectColorSwapchainFormatFunc selectColorSwapchainFormatCallback =
      nullptr;
  XrfwAllocateSwapchainImageStructsFunc allocateSwapchainImageStructsCallback =
      nullptr;
  std::vector<const char *> extensionNames;
  const void *next = nullptr;
};
