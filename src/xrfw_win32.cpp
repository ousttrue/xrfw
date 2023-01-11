#include "xrfw.h"
#include <algorithm>
#include <list>
#include <openxr/openxr_platform.h>
#include <plog/Log.h>
#include <unordered_map>

extern XrInstance g_instance;
extern XrSystemId g_systemId;
extern XrSession g_session;
extern XrSpace g_currentSpace;
extern XrSpace g_headSpace;
extern XrSpace g_localSpace;
extern XrCompositionLayerProjectionView g_projectionLayerViews[2];

std::list<std::vector<XrSwapchainImageOpenGLKHR>> g_swapchainImageBuffers;
struct SwapchainInfo {
  uint32_t index;
  int width;
  int height;
  std::vector<XrSwapchainImageBaseHeader *> images;
};
std::unordered_map<XrSwapchain, SwapchainInfo> g_swapchainImages;

bool _xrfwGraphicsRequirements() {

  // graphics
  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR =
      nullptr;
  auto result = xrGetInstanceProcAddr(
      g_instance, "xrGetOpenGLGraphicsRequirementsKHR",
      (PFN_xrVoidFunction *)(&pfnGetOpenGLGraphicsRequirementsKHR));
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetInstanceProcAddr: xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  XrGraphicsRequirementsOpenGLKHR graphicsRequirements = {
      .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
  };
  result = pfnGetOpenGLGraphicsRequirementsKHR(g_instance, g_systemId,
                                               &graphicsRequirements);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrGetOpenGLGraphicsRequirementsKHR";
    return false;
  }

  return true;
}

XRFW_API XrSession xrfwCreateOpenGLWin32SessionAndSwapchain(
    XrfwSwapchains *swapchains, HDC hDC, HGLRC hGLRC) {

  XrGraphicsBindingOpenGLWin32KHR graphicsBindingGL = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
      .next = nullptr,
      .hDC = hDC,
      .hGLRC = hGLRC,
  };

  XrSessionCreateInfo sessionCreateInfo = {
      .type = XR_TYPE_SESSION_CREATE_INFO,
      .next = &graphicsBindingGL,
      .createFlags = 0,
      .systemId = g_systemId,
  };

  auto result = xrCreateSession(g_instance, &sessionCreateInfo, &g_session);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrCreateSession: " << result;
    return nullptr;
  }

  // viewports
  {
    // Enumerate the viewport configurations.
    uint32_t viewportConfigTypeCount = 0;
    auto result = xrEnumerateViewConfigurations(g_instance, g_systemId, 0,
                                                &viewportConfigTypeCount, NULL);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrEnumerateViewConfigurations: " << result;
      return nullptr;
    }

    std::vector<XrViewConfigurationType> viewportConfigurationTypes(
        viewportConfigTypeCount);
    result = xrEnumerateViewConfigurations(
        g_instance, g_systemId, viewportConfigTypeCount,
        &viewportConfigTypeCount, viewportConfigurationTypes.data());
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrEnumerateViewConfigurations: " << result;
      return nullptr;
    }

    PLOG_INFO << "Available Viewport Configuration Types: "
              << viewportConfigTypeCount;
    for (uint32_t i = 0; i < viewportConfigTypeCount; i++) {
      const XrViewConfigurationType viewportConfigType =
          viewportConfigurationTypes[i];
      XrViewConfigurationProperties viewportConfig{
          .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES,
      };
      result = xrGetViewConfigurationProperties(
          g_instance, g_systemId, viewportConfigType, &viewportConfig);
      if (XR_FAILED(result)) {
        PLOG_FATAL << "xrGetViewConfigurationProperties: " << result;
        return nullptr;
      }
      PLOG_INFO << "  [" << i << "]FovMutable="
                << (viewportConfig.fovMutable ? "true" : "false")
                << " ConfigurationType " << viewportConfig.viewConfigurationType
                << (viewportConfigType == g_supportedViewConfigType
                        ? " Selected"
                        : "");

      uint32_t viewCount;
      result = xrEnumerateViewConfigurationViews(
          g_instance, g_systemId, viewportConfigType, 0, &viewCount, NULL);
      if (XR_FAILED(result)) {
        PLOG_FATAL << "xrEnumerateViewConfigurationViews: " << result;
        return nullptr;
      }
    }
  }

  // Get the viewport configuration info for the chosen viewport configuration
  // type.
  // g_viewportConfig = {.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
  // result = xrGetViewConfigurationProperties(
  //     g_instance, g_systemId, g_supportedViewConfigType, &g_viewportConfig);
  // if (XR_FAILED(result)) {
  //   PLOG_FATAL << "xrGetViewConfigurationProperties: " << result;
  //   return nullptr;
  // }
  // assert(g_viewportConfig.viewConfigurationType ==
  // g_supportedViewConfigType);

  // base space
  {
    XrReferenceSpaceCreateInfo spaceCreateInfo = {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
        .poseInReferenceSpace =
            {
                .orientation = {0, 0, 0, 1},
                .position = {0, 0, 0},
            },
    };
    auto result =
        xrCreateReferenceSpace(g_session, &spaceCreateInfo, &g_currentSpace);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return {};
    }
  }

  // head / local
  {
    XrReferenceSpaceCreateInfo spaceCreateInfo = {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
        .poseInReferenceSpace =
            {
                .orientation = {0, 0, 0, 1},
                .position = {0, 0, 0},
            },
    };
    auto result =
        xrCreateReferenceSpace(g_session, &spaceCreateInfo, &g_headSpace);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return {};
    }
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    result = xrCreateReferenceSpace(g_session, &spaceCreateInfo, &g_localSpace);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return {};
    }
  }

  // swapchain
  XrViewConfigurationView viewConfigurationViews[2];
  if (!xrfwGetViewConfigurationViews(viewConfigurationViews, 2)) {
    return {};
  }
  swapchains->left =
      xrfwCreateSwapchain(viewConfigurationViews[0], &swapchains->leftWidth,
                          &swapchains->leftHeight);
  if (!swapchains->left) {
    return {};
  }
  swapchains->right =
      xrfwCreateSwapchain(viewConfigurationViews[1], &swapchains->rightWidth,
                          &swapchains->rightHeight);
  if (!swapchains->right) {
    return {};
  }

  return g_session;
}

static int64_t SelectColorSwapchainFormat(XrSession session) {
  // Select a swapchain format.
  uint32_t swapchainFormatCount;
  auto result =
      xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, nullptr);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrEnumerateSwapchainFormats");
  }

  // std::vector<int64_t> runtimeFormats;
  std::vector<int64_t> swapchainFormats(swapchainFormatCount);
  result = xrEnumerateSwapchainFormats(
      session, (uint32_t)swapchainFormats.size(), &swapchainFormatCount,
      swapchainFormats.data());
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrEnumerateSwapchainFormats");
  }
  assert(swapchainFormatCount == swapchainFormats.size());

  // List of supported color swapchain formats.
  constexpr int64_t SupportedColorSwapchainFormats[] = {
      0x8059, // GL_RGB10_A2,
      0x1908, // GL_RGBA16F,
      // The two below should only be used as a fallback, as they are linear
      // color formats without enough bits for color depth, thus leading to
      // banding.
      0x8058, // GL_RGBA8,
      0x8F97, // GL_RGBA8_SNORM,
  };

  auto swapchainFormatIt =
      std::find_first_of(swapchainFormats.begin(), swapchainFormats.end(),
                         std::begin(SupportedColorSwapchainFormats),
                         std::end(SupportedColorSwapchainFormats));
  if (swapchainFormatIt == swapchainFormats.end()) {
    throw std::runtime_error(
        "No runtime swapchain format supported for color swapchain");
  }
  return *swapchainFormatIt;
}

static std::vector<XrSwapchainImageBaseHeader *> AllocateSwapchainImageStructs(
    uint32_t capacity, const XrSwapchainCreateInfo & /*swapchainCreateInfo*/) {
  // Allocate and initialize the buffer of image structs (must be sequential in
  // memory for xrEnumerateSwapchainImages). Return back an array of pointers to
  // each swapchain image struct so the consumer doesn't need to know the
  // type/size.
  std::vector<XrSwapchainImageOpenGLKHR> swapchainImageBuffer(capacity);
  std::vector<XrSwapchainImageBaseHeader *> swapchainImageBase;
  for (XrSwapchainImageOpenGLKHR &image : swapchainImageBuffer) {
    image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
    swapchainImageBase.push_back(
        reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
  }

  // Keep the buffer alive by moving it into the list of buffers.
  g_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));

  return swapchainImageBase;
}

XRFW_API XrSwapchain
xrfwCreateSwapchain(const XrViewConfigurationView &viewConfigurationView,
                    int *width, int *height) {
  auto colorSwapchainFormat = SelectColorSwapchainFormat(g_session);

  PLOG_INFO << "Creating swapchain for view "
            << "with dimensions Width="
            << viewConfigurationView.recommendedImageRectWidth
            << " Height=" << viewConfigurationView.recommendedImageRectHeight
            << " Format=" << colorSwapchainFormat << " SampleCount="
            << viewConfigurationView.recommendedSwapchainSampleCount;

  // Create the swapchain.
  XrSwapchainCreateInfo swapchainCreateInfo{
      .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
      .next = nullptr,
      .createFlags = 0,
      .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                    XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
      .format = colorSwapchainFormat,
      .sampleCount = viewConfigurationView.recommendedSwapchainSampleCount,
      .width = viewConfigurationView.recommendedImageRectWidth,
      .height = viewConfigurationView.recommendedImageRectHeight,
      .faceCount = 1,
      .arraySize = 1,
      .mipCount = 1,
  };

  XrSwapchain swapchain;
  auto result = xrCreateSwapchain(g_session, &swapchainCreateInfo, &swapchain);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return {};
  }

  uint32_t imageCount;
  result = xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return {};
  }

  // XXX This should really just return XrSwapchainImageBaseHeader*
  auto swapchainImages =
      AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);
  result = xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount,
                                      swapchainImages[0]);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return {};
  }

  auto index = (uint32_t)g_swapchainImages.size();
  g_swapchainImages.insert(std::make_pair(
      swapchain,
      SwapchainInfo{index, static_cast<int>(swapchainCreateInfo.width),
                    static_cast<int>(swapchainCreateInfo.height),
                    swapchainImages}));

  *width = swapchainCreateInfo.width;
  *height = swapchainCreateInfo.height;
  return swapchain;
}

XRFW_API const XrSwapchainImageBaseHeader *
xrfwAcquireSwapchain(XrSwapchain swapchain) {
  auto found = g_swapchainImages.find(swapchain);
  if (found == g_swapchainImages.end()) {
    return {};
  }
  auto &info = found->second;
  g_projectionLayerViews[info.index].subImage = {
      .swapchain = swapchain,
      .imageRect =
          {
              .offset = {0, 0},
              .extent = {info.width, info.height},
          },
  };

  XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
  uint32_t swapchainImageIndex;
  auto result =
      xrAcquireSwapchainImage(swapchain, &acquireInfo, &swapchainImageIndex);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrAcquireSwapchainImage: " << result;
    return {};
  }

  XrSwapchainImageWaitInfo waitInfo{
      .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
      .timeout = XR_INFINITE_DURATION,
  };
  result = xrWaitSwapchainImage(swapchain, &waitInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrWaitSwapchainImage: " << result;
    return {};
  }

  return g_swapchainImages[swapchain].images[swapchainImageIndex];
}

XRFW_API void xrfwReleaseSwapchain(XrSwapchain swapchain) {
  XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  auto result = xrReleaseSwapchainImage(swapchain, &releaseInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrReleaseSwapchainImage: " << result;
  }
}
