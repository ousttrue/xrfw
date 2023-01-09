#include "oxrrenderer.h"
#include "openxr/openxr.h"
#include <optional>
#include <stdexcept>
#include <xrfw.h>
#include <plog/Log.h>

OxrRenderer::OxrRenderer(XrInstance instance, XrSession session)
    : instance_(instance), session_(session) {}

OxrRenderer::~OxrRenderer() {}

bool OxrRenderer::Initialize() {
  // prepare
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
        xrCreateReferenceSpace(session_, &spaceCreateInfo, &currentSpace_);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return false;
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
        xrCreateReferenceSpace(session_, &spaceCreateInfo, &headSpace_);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return false;
    }
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    result = xrCreateReferenceSpace(session_, &spaceCreateInfo, &localSpace_);
    if (XR_FAILED(result)) {
      PLOG_FATAL << result;
      return false;
    }
  }

  return true;
}

void OxrRenderer::RenderFrame() {

  // const XrCompositionLayerBaseHeader *layers[] = {nullptr};
  // std::optional<XrCompositionLayerProjection> layer;
  // if (frameState.shouldRender == XR_TRUE) {
  //   layer = RenderLayer(frameState.predictedDisplayTime);
  //   if (layer) {
  //     layers[0] = (const XrCompositionLayerBaseHeader *)&layer.value();
  //   }
  // }

  // auto pLayer = (layer ? &layer.value() : nullptr);
}

std::optional<XrCompositionLayerProjection>
OxrRenderer::RenderLayer(XrTime predictedDisplayTime) {

  XrViewState viewState{XR_TYPE_VIEW_STATE};

  XrViewLocateInfo viewLocateInfo{
      .type = XR_TYPE_VIEW_LOCATE_INFO,
      .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
      .displayTime = predictedDisplayTime,
      .space = currentSpace_,
  };

  XrView views[2]{
      {XR_TYPE_VIEW},
      {XR_TYPE_VIEW},
  };
  uint32_t viewCountOutput;
  auto result = xrLocateViews(session_, &viewLocateInfo, &viewState, 2,
                              &viewCountOutput, views);
  if (XR_FAILED(result)) {
    PLOG_FATAL << "xrLocateViews: " << result;
    return {};
  }
  if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
      (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
    return {}; // There is no valid tracking poses for the views.
  }

  assert(viewCountOutput == 2);

  // Render view to the appropriate part of the swapchain image.
  XrCompositionLayerProjectionView projectionLayerViews[2];
  for (uint32_t i = 0; i < viewCountOutput; i++) {
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

    projectionLayerViews[i] = {
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
      .space = currentSpace_,
      .viewCount = 2,
      .views = projectionLayerViews,
  };
}

