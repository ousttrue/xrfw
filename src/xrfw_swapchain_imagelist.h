#pragma once
#include <list>
#include <openxr/openxr.h>
#include <vector>

template <typename T, XrStructureType TYPE> struct SwapchainImageList {
  static std::list<std::vector<T>> s_swapchainImageBuffers;
  static std::vector<XrSwapchainImageBaseHeader *>
  allocateSwapchainImageStructs(
      uint32_t size, const XrSwapchainCreateInfo & /*swapchainCreateInfo*/) {
    // Allocate and initialize the buffer of image structs (must be sequential
    // in memory for xrEnumerateSwapchainImages). Return back an array of
    // pointers to each swapchain image struct so the consumer doesn't need to
    // know the type/size.
    std::vector<T> swapchainImageBuffer(size);
    std::vector<XrSwapchainImageBaseHeader *> swapchainImageBase;
    for (T &image : swapchainImageBuffer) {
      image.type = TYPE; // XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
      swapchainImageBase.push_back(
          reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
    }
    // Keep the buffer alive by moving it into the list of buffers.
    s_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));
    return swapchainImageBase;
  }
};
