#include "oxrapp.h"
#include <stdexcept>
#include <xrfw.h>

#include <plog/Log.h>

static const int MAX_NUM_EYES = 2;

OxrApp::OxrApp(XrInstance instance, XrSession session)
    : instance_(instance), session_(session) {}

OxrApp::~OxrApp() {}

bool OxrApp::Initialize() {
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

void OxrApp::ProcessFrame() {
  // frame
  XrFrameWaitInfo waitFrameInfo = {
      .type = XR_TYPE_FRAME_WAIT_INFO,
      .next = nullptr,
  };
  XrFrameState frameState = {
      .type = XR_TYPE_FRAME_STATE,
      .next = nullptr,
  };
  auto result = xrWaitFrame(session_, &waitFrameInfo, &frameState);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrWaitFrame");
  }

  XrFrameBeginInfo beginFrameDesc = {
      .type = XR_TYPE_FRAME_BEGIN_INFO,
      .next = nullptr,
  };
  result = xrBeginFrame(session_, &beginFrameDesc);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrBeginFrame");
  }

  // location
  XrSpaceLocation loc = {
      .type = XR_TYPE_SPACE_LOCATION,
  };
  result = xrLocateSpace(headSpace_, currentSpace_,
                         frameState.predictedDisplayTime, &loc);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrLocateSpace");
  }

  XrPosef xfStageFromHead = loc.pose;
  result = xrLocateSpace(headSpace_, localSpace_,
                         frameState.predictedDisplayTime, &loc);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrLocateSpace");
  }

  // views
  XrViewState viewState = {
      .type = XR_TYPE_VIEW_STATE,
      .next = nullptr,
  };
  XrViewLocateInfo projectionInfo = {
      .type = XR_TYPE_VIEW_LOCATE_INFO,
      .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
      .displayTime = frameState.predictedDisplayTime,
      .space = headSpace_,
  };

  uint32_t projectionCapacityInput = MAX_NUM_EYES;
  uint32_t projectionCountOutput = projectionCapacityInput;
  result = xrLocateViews(session_, &projectionInfo, &viewState,
                         projectionCapacityInput, &projectionCountOutput,
                         projections);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrLocateViews");
  }

  XrPosef viewTransform[2];
  for (int eye = 0; eye < MAX_NUM_EYES; eye++) {
    //   XrPosef xfHeadFromEye = Projections[eye].pose;
    //   XrPosef xfStageFromEye = XrPosef_Multiply(xfStageFromHead,
    //   xfHeadFromEye); viewTransform[eye] =
    //   XrPosef_Inverse(xfStageFromEye); XrMatrix4x4f viewMat =
    //       XrMatrix4x4f_CreateFromRigidTransform(&viewTransform[eye]);
    //   const XrFovf fov = Projections[eye].fov;
    //   XrMatrix4x4f projMat;
    //   XrMatrix4x4f_CreateProjectionFov(&projMat, GRAPHICS_OPENGL_ES, fov,
    //   0.1f,
    //                                    0.0f);
    //   out.FrameMatrices.EyeView[eye] =
    //   XrMatrix4x4f_To_OVRMatrix4f(viewMat);
    //   out.FrameMatrices.EyeProjection[eye] =
    //       XrMatrix4x4f_To_OVRMatrix4f(projMat);
    //   in.Eye[eye].ViewMatrix = out.FrameMatrices.EyeView[eye];
    //   in.Eye[eye].ProjectionMatrix =
    //   out.FrameMatrices.EyeProjection[eye];
  }

  //
  // TODO: render
  //

  // XrPosef centerView = XrPosef_Inverse(xfStageFromHead);
  // XrMatrix4x4f viewMat =
  // XrMatrix4x4f_CreateFromRigidTransform(&centerView);
  // out.FrameMatrices.CenterView = XrMatrix4x4f_To_OVRMatrix4f(viewMat);

  // Set-up the compositor layers for this frame.
  // NOTE: Multiple independent layers are allowed, but they need to be
  // added
  // in a depth consistent order.
  // XrCompositionLayerProjectionView projection_layer_elements[2] = {};
  // auto LayerCount = 0;
  std::vector<XrCompositionLayerBaseHeader *> layers;

  // Render the world-view layer (projection)
  {
    //   AppRenderFrame(in, out);

    //   XrCompositionLayerProjection projection_layer = {};
    //   projection_layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    //   projection_layer.layerFlags =
    //       XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    //   projection_layer.layerFlags |=
    //       XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
    //   projection_layer.space = CurrentSpace;
    //   projection_layer.viewCount = MAX_NUM_EYES;
    //   projection_layer.views = projection_layer_elements;

    //   for (int eye = 0; eye < MAX_NUM_EYES; eye++) {
    //     ovrFramebuffer *frameBuffer = &FrameBuffer[eye];
    //     memset(&projection_layer_elements[eye], 0,
    //            sizeof(XrCompositionLayerProjectionView));
    //     projection_layer_elements[eye].type =
    //         XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
    //     projection_layer_elements[eye].pose =
    //         XrPosef_Inverse(viewTransform[eye]);
    //     projection_layer_elements[eye].fov = Projections[eye].fov;
    //     memset(&projection_layer_elements[eye].subImage, 0,
    //            sizeof(XrSwapchainSubImage));
    //     projection_layer_elements[eye].subImage.swapchain =
    //         frameBuffer->ColorSwapChain.Handle;
    //     projection_layer_elements[eye].subImage.imageRect.offset.x = 0;
    //     projection_layer_elements[eye].subImage.imageRect.offset.y = 0;
    //     projection_layer_elements[eye].subImage.imageRect.extent.width =
    //         frameBuffer->ColorSwapChain.Width;
    //     projection_layer_elements[eye].subImage.imageRect.extent.height =
    //         frameBuffer->ColorSwapChain.Height;
    //     projection_layer_elements[eye].subImage.imageArrayIndex = 0;
    //   }

    //   Layers[LayerCount++].Projection = projection_layer;
  }

  XrFrameEndInfo endFrameInfo = {
      .type = XR_TYPE_FRAME_END_INFO,
      .displayTime = frameState.predictedDisplayTime,
      .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
      .layerCount = static_cast<uint32_t>(layers.size()),
      .layers = layers.data(),
  };
  result = xrEndFrame(session_, &endFrameInfo);
  if (XR_FAILED(result)) {
    PLOG_FATAL << result;
    throw std::runtime_error("xrEndFrame");
  }
}

void OxrApp::RenderFrame() {

  // XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  // XrFrameState frameState{XR_TYPE_FRAME_STATE};
  // CHECK_XRCMD(xrWaitFrame(session_, &frameWaitInfo, &frameState));

  // XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
  // CHECK_XRCMD(xrBeginFrame(session_, &frameBeginInfo));

  // std::vector<XrCompositionLayerBaseHeader *> layers;
  // XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  // std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
  // if (frameState.shouldRender == XR_TRUE) {
  //   if (RenderLayer(frameState.predictedDisplayTime, projectionLayerViews,
  //                   layer)) {
  //     layers.push_back(
  //         reinterpret_cast<XrCompositionLayerBaseHeader *>(&layer));
  //   }
  // }

  // XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
  // frameEndInfo.displayTime = frameState.predictedDisplayTime;
  // frameEndInfo.environmentBlendMode = m_options->Parsed.EnvironmentBlendMode;
  // frameEndInfo.layerCount = (uint32_t)layers.size();
  // frameEndInfo.layers = layers.data();
  // CHECK_XRCMD(xrEndFrame(session_, &frameEndInfo));
}
