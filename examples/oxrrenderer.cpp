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

static int64_t
SelectColorSwapchainFormat(const std::vector<int64_t> &runtimeFormats) {
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
      std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(),
                         std::begin(SupportedColorSwapchainFormats),
                         std::end(SupportedColorSwapchainFormats));
  if (swapchainFormatIt == runtimeFormats.end()) {
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

bool OxrRenderer::CreateSwapchains(int viewCount) {

  // Select a swapchain format.
  uint32_t swapchainFormatCount;
  auto result =
      xrEnumerateSwapchainFormats(session_, 0, &swapchainFormatCount, nullptr);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return false;
  }

  std::vector<int64_t> swapchainFormats(swapchainFormatCount);
  result = xrEnumerateSwapchainFormats(
      session_, (uint32_t)swapchainFormats.size(), &swapchainFormatCount,
      swapchainFormats.data());
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    return false;
  }
  assert(swapchainFormatCount == swapchainFormats.size());

  auto colorSwapchainFormat = SelectColorSwapchainFormat(swapchainFormats);

  // Print swapchain formats and the selected one.
  {
    std::string swapchainFormatsString;
    for (int64_t format : swapchainFormats) {
      const bool selected = format == colorSwapchainFormat;
      swapchainFormatsString += " ";
      if (selected) {
        swapchainFormatsString += "[";
      }
      swapchainFormatsString += ToGLString(format);
      if (selected) {
        swapchainFormatsString += "]";
      }
    }
    PLOG_DEBUG << "Swapchain Formats: " << swapchainFormatsString;
  }

  //   // Create a swapchain for each view.
  //   for (uint32_t i = 0; i < viewCount; i++) {
  //     const XrViewConfigurationView &vp = m_configViews[i];
  //     Log::Write(Log::Level::Info,
  //                Fmt("Creating swapchain for view %d with dimensions Width=%d
  //                "
  //                    "Height=%d SampleCount=%d",
  //                    i, vp.recommendedImageRectWidth,
  //                    vp.recommendedImageRectHeight,
  //                    vp.recommendedSwapchainSampleCount));

  //     // Create the swapchain.
  //     XrSwapchainCreateInfo
  //     swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
  //     swapchainCreateInfo.arraySize = 1;
  //     swapchainCreateInfo.format = m_colorSwapchainFormat;
  //     swapchainCreateInfo.width = vp.recommendedImageRectWidth;
  //     swapchainCreateInfo.height = vp.recommendedImageRectHeight;
  //     swapchainCreateInfo.mipCount = 1;
  //     swapchainCreateInfo.faceCount = 1;
  //     swapchainCreateInfo.sampleCount =
  //         m_graphicsPlugin->GetSupportedSwapchainSampleCount(vp);
  //     swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
  //                                      XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
  //     Swapchain swapchain;
  //     swapchain.width = swapchainCreateInfo.width;
  //     swapchain.height = swapchainCreateInfo.height;
  //     CHECK_XRCMD(xrCreateSwapchain(m_session, &swapchainCreateInfo,
  //                                   &swapchain.handle));

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
  //   }

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
