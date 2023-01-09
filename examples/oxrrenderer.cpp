#include "oxrrenderer.h"
#include "openxr/openxr.h"
#include <algorithm>
#include <gl/glew.h>
#include <optional>
#include <plog/Log.h>
#include <stdexcept>
#include <stdint.h>
#include <unordered_map>
#include <xrfw.h>

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
      GL_RGB10_A2,
      GL_RGBA16F,
      // The two below should only be used as a fallback, as they are linear
      // color formats without enough bits for color depth, thus leading to
      // banding.
      GL_RGBA8,
      GL_RGBA8_SNORM,
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

std::unordered_map<uint64_t, const char *> g_glNameMap{
    {GL_RGBA8, "GL_RGBA8"},
    {GL_RGB16F, "GL_RGB16F"},
    {GL_R11F_G11F_B10F_EXT, "GL_R11F_G11F_B10F_EXT"},
    {GL_SRGB8_ALPHA8_EXT, "GL_SRGB8_ALPHA8_EXT"},
    {GL_DEPTH_COMPONENT16, "GL_DEPTH_COMPONENT16"},
    {GL_DEPTH_COMPONENT24, "GL_DEPTH_COMPONENT24"},
    {GL_DEPTH_COMPONENT32, "GL_DEPTH_COMPONENT32"},
    {GL_DEPTH24_STENCIL8, "GL_DEPTH24_STENCIL8"},
    {GL_DEPTH_COMPONENT32F, "GL_DEPTH_COMPONENT32F"},
    {GL_DEPTH32F_STENCIL8, "GL_DEPTH32F_STENCIL8"},
};

static std::string_view ToGLString(uint64_t value) {
  auto found = g_glNameMap.find(value);
  if (found != g_glNameMap.end()) {
    return found->second;
  }
  PLOG_FATAL << "0x" << std::hex << value;
  throw std::runtime_error("unknown");
}

OxrRenderer::OxrRenderer(XrInstance instance, XrSession session)
    : instance_(instance), session_(session) {}

OxrRenderer::~OxrRenderer() {}

bool OxrRenderer::CreateSwapchain(const XrViewConfigurationView &vp) {

  auto colorSwapchainFormat = SelectColorSwapchainFormat(session_);

  PLOG_INFO << "Creating swapchain for view "
            << "with dimensions Width=" << vp.recommendedImageRectWidth
            << " Height=" << vp.recommendedImageRectHeight
            << " Format=" << colorSwapchainFormat
            << " SampleCount=" << vp.recommendedSwapchainSampleCount;

  // Create the swapchain.
  XrSwapchainCreateInfo swapchainCreateInfo{
      .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
      .next = nullptr,
      .createFlags = 0,
      .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                    XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
      .format = colorSwapchainFormat,
      .sampleCount = vp.recommendedSwapchainSampleCount,
      .width = vp.recommendedImageRectWidth,
      .height = vp.recommendedImageRectHeight,
      .faceCount = 1,
      .arraySize = 1,
      .mipCount = 1,
  };

  XrSwapchain swapchain;
  auto result = xrCreateSwapchain(session_, &swapchainCreateInfo, &swapchain);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return false;
  }

  //     m_swapchains.push_back(swapchain);

  //     uint32_t imageCount;
  //     CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, 0,
  //     &imageCount,
  //                                            nullptr));
  //     // XXX This should really just return XrSwapchainImageBaseHeader*
  //     std::vector<XrSwapchainImageBaseHeader *> swapchainImages =
  //         m_graphicsPlugin->AllocateSwapchainImageStructs(imageCount,
  //                                                         swapchainCreateInfo);
  //     CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, imageCount,
  //                                            &imageCount,
  //                                            swapchainImages[0]));

  //     m_swapchainImages.insert(
  //         std::make_pair(swapchain.handle, std::move(swapchainImages)));

  return false;
}

std::optional<XrCompositionLayerProjection>
OxrRenderer::RenderLayer(XrTime predictedDisplayTime, const XrView views[2]) {
  // Render view to the appropriate part of the swapchain image.
  for (uint32_t i = 0; i < 2; i++) {
    // Each view has a separate swapchain which is acquired, rendered to, and
    // released.
    auto viewSwapchain = swapchains_[i];

    XrSwapchainImageAcquireInfo acquireInfo{
        XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t swapchainImageIndex;
    auto result = xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo,
                                          &swapchainImageIndex);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrAcquireSwapchainImage: " << result;
      return {};
    }

    XrSwapchainImageWaitInfo waitInfo{
        .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
        .timeout = XR_INFINITE_DURATION,
    };
    result = xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrWaitSwapchainImage: " << result;
      return {};
    }

    projectionLayerViews_[i] = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
        .pose = views[i].pose,
        .fov = views[i].fov,
        .subImage =
            {
                .swapchain = viewSwapchain.handle,
                .imageRect =
                    {
                        .offset = {0, 0},
                        .extent = {viewSwapchain.width, viewSwapchain.height},
                    },
            },
    };

    //   const XrSwapchainImageBaseHeader *const swapchainImage =
    //       m_swapchainImages[viewSwapchain.handle][swapchainImageIndex];
    //   m_graphicsPlugin->RenderView(projectionLayerViews[i], swapchainImage,
    //                                m_colorSwapchainFormat, cubes);

    XrSwapchainImageReleaseInfo releaseInfo{
        XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    result = xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo);
    if (XR_FAILED(result)) {
      PLOG_FATAL << "xrReleaseSwapchainImage: " << result;
      return {};
    }
  }

  return XrCompositionLayerProjection{
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
      .next = nullptr,
      .layerFlags = 0,
      .space = {},
      .viewCount = 2,
      .views = projectionLayerViews_,
  };
}
