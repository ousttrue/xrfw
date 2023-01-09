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

std::optional<XrCompositionLayerProjectionView>
OxrRenderer::RenderLayer(XrSwapchain swapchain, int width, int height,
                         XrTime predictedDisplayTime, const XrView &view) {

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

  //   const XrSwapchainImageBaseHeader *const swapchainImage =
  //       m_swapchainImages[viewSwapchain.handle][swapchainImageIndex];
  //   m_graphicsPlugin->RenderView(projectionLayerViews[i], swapchainImage,
  //                                m_colorSwapchainFormat, cubes);

  XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  result = xrReleaseSwapchainImage(swapchain, &releaseInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrReleaseSwapchainImage: " << result;
    return {};
  }

  return XrCompositionLayerProjectionView{
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
      .pose = view.pose,
      .fov = view.fov,
      .subImage =
          {
              .swapchain = swapchain,
              .imageRect =
                  {
                      .offset = {0, 0},
                      .extent = {width, height},
                  },
          },
  };
}
