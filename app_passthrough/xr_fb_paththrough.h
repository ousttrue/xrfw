#pragma once
#include <span>
#include <xrfw_proc.h>

struct FBPassthrough
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_FB_PASSTHROUGH_EXTENSION_NAME,
      XR_FB_TRIANGLE_MESH_EXTENSION_NAME // optional
    };
    return s_extensions;
  }
  static bool SystemSupportsPassthrough(XrInstance instance,
                                        XrFormFactor formFactor)
  {
    XrSystemPassthroughProperties2FB passthroughSystemProperties{
      XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES2_FB
    };
    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES,
                                         &passthroughSystemProperties };

    XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
    systemGetInfo.formFactor = formFactor;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance, &systemGetInfo, &systemId);
    xrGetSystemProperties(instance, systemId, &systemProperties);

    // XR_PASSTHROUGH_CAPABILITY_COLOR_BIT_FB
    return (passthroughSystemProperties.capabilities &
            XR_PASSTHROUGH_CAPABILITY_BIT_FB) ==
           XR_PASSTHROUGH_CAPABILITY_BIT_FB;
  }

  XRFW_PROC(xrCreatePassthroughFB);
  XRFW_PROC(xrCreatePassthroughLayerFB);
  XRFW_PROC(xrPassthroughLayerSetStyleFB);
  FBPassthrough(XrInstance instance, XrSystemId system)
    : xrCreatePassthroughFB(instance)
    , xrCreatePassthroughLayerFB(instance)
    , xrPassthroughLayerSetStyleFB(instance)
  {
  }
};

struct FBPassthroughFeature
{
  const FBPassthrough& m_ext;
  XrSession m_session;
  XrPassthroughFB m_passthroughFeature = XR_NULL_HANDLE;
  XrPassthroughLayerFB m_passthroughLayer = XR_NULL_HANDLE;
  XrCompositionLayerPassthroughFB m_passthroughCompLayer = {};

  FBPassthroughFeature(const FBPassthrough& ext, XrSession session)
    : m_ext(ext)
    , m_session(session)
  {
    XrPassthroughCreateInfoFB passthroughCreateInfo = {
      .type = XR_TYPE_PASSTHROUGH_CREATE_INFO_FB,
      .flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB,
    };

    XrResult result = m_ext.xrCreatePassthroughFB(
      session, &passthroughCreateInfo, &m_passthroughFeature);
    if (XR_FAILED(result)) {
      PLOG_ERROR << "Failed to create a passthrough feature.";
    }
  }

  void CreateLayer()
  {
    // Create and run passthrough layer

    XrPassthroughLayerCreateInfoFB layerCreateInfo = {
      .type = XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB,
      .passthrough = m_passthroughFeature,
      .flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB,
      .purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB,
    };

    XrResult result = m_ext.xrCreatePassthroughLayerFB(
      m_session, &layerCreateInfo, &m_passthroughLayer);
    if (XR_FAILED(result)) {
      PLOG_ERROR << "Failed to create and start a passthrough layer";
    }

    m_passthroughCompLayer = {
      .type = XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB,
      .flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT,
      .space = XR_NULL_HANDLE,
      .layerHandle = m_passthroughLayer,
    };

    XrPassthroughStyleFB style{
      .type = XR_TYPE_PASSTHROUGH_STYLE_FB,
      .textureOpacityFactor = 0.5f,
      .edgeColor = { 0.0f, 0.0f, 0.0f, 0.0f },
      // .next = &colorMap,
    };
    if (XR_FAILED(
          m_ext.xrPassthroughLayerSetStyleFB(m_passthroughLayer, &style))) {
      PLOG_ERROR << "xrPassthroughLayerSetStyleFB";
    }
  }

  // void Start()
  // {
  //   // Start the feature manually
  //   XrResult result = m_ext.pfnXrPassthroughStartFB(passthroughFeature);
  //   if (XR_FAILED(result)) {
  //     LOG("Failed to start a passthrough feature.\n");
  //   }
  // }
};
