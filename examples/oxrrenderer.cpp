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

  // XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  // std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
  if (frameState.shouldRender == XR_TRUE) {
    //   if (RenderLayer(frameState.predictedDisplayTime, projectionLayerViews,
    //                   layer)) {
    //     layers.push_back(
    //         reinterpret_cast<XrCompositionLayerBaseHeader *>(&layer));
    //   }
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
