#include "oxrrenderer.h"
#include "openxr/openxr.h"
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
  XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState frameState{XR_TYPE_FRAME_STATE};
  auto result = xrWaitFrame(session_, &frameWaitInfo, &frameState);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrWaitFrame");
  }

  XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
  result = xrBeginFrame(session_, &frameBeginInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrBeginFrame");
  }

  // std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
  if (frameState.shouldRender == XR_TRUE) {
    XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    if (RenderLayer(frameState.predictedDisplayTime, layer)) {
      layers.push_back(
          reinterpret_cast<XrCompositionLayerBaseHeader *>(&layer));
    }
  }

  XrFrameEndInfo frameEndInfo{
      .type = XR_TYPE_FRAME_END_INFO,
      .displayTime = frameState.predictedDisplayTime,
      .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
      .layerCount = (uint32_t)layers.size(),
      .layers = layers.data(),
  };
  result = xrEndFrame(session_, &frameEndInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrEndFrame");
  }
}

bool OxrRenderer::RenderLayer(XrTime predictedDisplayTime,
                              XrCompositionLayerProjection &layer) {

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
    return false;
  }
  if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
      (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
    return false; // There is no valid tracking poses for the views.
  }

  // CHECK(viewCountOutput == viewCapacityInput);
  // CHECK(viewCountOutput == m_configViews.size());
  // CHECK(viewCountOutput == m_swapchains.size());

  // projectionLayerViews.resize(viewCountOutput);

  // // For each locatable space that we want to visualize, render a 25cm cube.
  // std::vector<Cube> cubes;

  // for (XrSpace visualizedSpace : m_visualizedSpaces) {
  //   XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
  //   res = xrLocateSpace(visualizedSpace, m_appSpace, predictedDisplayTime,
  //                       &spaceLocation);
  //   CHECK_XRRESULT(res, "xrLocateSpace");
  //   if (XR_UNQUALIFIED_SUCCESS(res)) {
  //     if ((spaceLocation.locationFlags &
  //          XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
  //         (spaceLocation.locationFlags &
  //          XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
  //       cubes.push_back(Cube{spaceLocation.pose, {0.25f, 0.25f, 0.25f}});
  //     }
  //   } else {
  //     Log::Write(
  //         Log::Level::Verbose,
  //         Fmt("Unable to locate a visualized reference space in app space:
  //         %d",
  //             res));
  //   }
  // }

  // // Render a 10cm cube scaled by grabAction for each hand. Note renderHand
  // will
  // // only be true when the application has focus.
  // for (auto hand : {Side::LEFT, Side::RIGHT}) {
  //   XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
  //   res = xrLocateSpace(m_input.handSpace[hand], m_appSpace,
  //                       predictedDisplayTime, &spaceLocation);
  //   CHECK_XRRESULT(res, "xrLocateSpace");
  //   if (XR_UNQUALIFIED_SUCCESS(res)) {
  //     if ((spaceLocation.locationFlags &
  //          XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
  //         (spaceLocation.locationFlags &
  //          XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
  //       float scale = 0.1f * m_input.handScale[hand];
  //       cubes.push_back(Cube{spaceLocation.pose, {scale, scale, scale}});
  //     }
  //   } else {
  //     // Tracking loss is expected when the hand is not active so only log a
  //     // message if the hand is active.
  //     if (m_input.handActive[hand] == XR_TRUE) {
  //       const char *handName[] = {"left", "right"};
  //       Log::Write(Log::Level::Verbose,
  //                  Fmt("Unable to locate %s hand action space in app space:
  //                  %d",
  //                      handName[hand], res));
  //     }
  //   }
  // }

  // // Render view to the appropriate part of the swapchain image.
  // for (uint32_t i = 0; i < viewCountOutput; i++) {
  //   // Each view has a separate swapchain which is acquired, rendered to, and
  //   // released.
  //   const Swapchain viewSwapchain = m_swapchains[i];

  //   XrSwapchainImageAcquireInfo acquireInfo{
  //       XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

  //   uint32_t swapchainImageIndex;
  //   CHECK_XRCMD(xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo,
  //                                       &swapchainImageIndex));

  //   XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
  //   waitInfo.timeout = XR_INFINITE_DURATION;
  //   CHECK_XRCMD(xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo));

  //   projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
  //   projectionLayerViews[i].pose = m_views[i].pose;
  //   projectionLayerViews[i].fov = m_views[i].fov;
  //   projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
  //   projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
  //   projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width,
  //                                                        viewSwapchain.height};

  //   const XrSwapchainImageBaseHeader *const swapchainImage =
  //       m_swapchainImages[viewSwapchain.handle][swapchainImageIndex];
  //   m_graphicsPlugin->RenderView(projectionLayerViews[i], swapchainImage,
  //                                m_colorSwapchainFormat, cubes);

  //   XrSwapchainImageReleaseInfo releaseInfo{
  //       XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  //   CHECK_XRCMD(xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo));
  // }

  // layer.space = m_appSpace;
  // layer.layerFlags = m_options->Parsed.EnvironmentBlendMode ==
  //                            XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND
  //                        ?
  //                        XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
  //                        |
  //                              XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT
  //                        : 0;
  // layer.viewCount = (uint32_t)projectionLayerViews.size();
  // layer.views = projectionLayerViews.data();
  // return true;
  return false;
}
