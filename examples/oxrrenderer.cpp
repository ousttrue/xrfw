#include "oxrrenderer.h"
#include <optional>
#include <plog/Log.h>
#include <stdexcept>
#include <xrfw.h>

OxrRenderer::OxrRenderer(XrInstance instance, XrSession session)
    : instance_(instance), session_(session) {}

OxrRenderer::~OxrRenderer() {}

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
