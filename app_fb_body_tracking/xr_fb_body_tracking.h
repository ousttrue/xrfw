#pragma once
#include <DirectXMath.h>
#include <functional>
#include <openxr/openxr.h>
#include <plog/Log.h>
#include <span>
#include <vector>

struct FbBodyTracking
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_FB_BODY_TRACKING_EXTENSION_NAME,
    };
    return s_extensions;
  }
  PFN_xrCreateBodyTrackerFB xrCreateBodyTrackerFB_ = nullptr;
  PFN_xrDestroyBodyTrackerFB xrDestroyBodyTrackerFB_ = nullptr;
  PFN_xrLocateBodyJointsFB xrLocateBodyJointsFB_ = nullptr;
  FbBodyTracking(XrInstance instance, XrSystemId system)
  {
    if (XR_FAILED(xrGetInstanceProcAddr(
          instance,
          "xrCreateBodyTrackerFB",
          (PFN_xrVoidFunction*)(&xrCreateBodyTrackerFB_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrCreateBodyTrackerFB";
    }
    if (XR_FAILED(xrGetInstanceProcAddr(
          instance,
          "xrDestroyBodyTrackerFB",
          (PFN_xrVoidFunction*)(&xrDestroyBodyTrackerFB_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrDestroyBodyTrackerFB";
    }
    if (XR_FAILED(xrGetInstanceProcAddr(
          instance,
          "xrLocateBodyJointsFB",
          (PFN_xrVoidFunction*)(&xrLocateBodyJointsFB_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrLocateBodyJointsFB";
    }
  }
};

struct FbBodyTracker
{
  const FbBodyTracking& m_ext;
  XrBodyTrackerFB m_tracker = XR_NULL_HANDLE;
  XrBodyJointLocationFB m_jointLocations[XR_BODY_JOINT_COUNT_FB];
  XrBodyJointLocationsFB m_locations = {};

  FbBodyTracker(const FbBodyTracking& ext, XrSession session)
    : m_ext(ext)
  {
    XrBodyTrackerCreateInfoFB createInfo{
      .type = XR_TYPE_BODY_TRACKER_CREATE_INFO_FB,
      .bodyJointSet = XR_BODY_JOINT_SET_DEFAULT_FB,
    };
    if (XR_FAILED(
          m_ext.xrCreateBodyTrackerFB_(session, &createInfo, &m_tracker))) {
      PLOG_ERROR << "xrCreateBodyTrackerFB_";
    }
  }

  ~FbBodyTracker()
  {
    if (XR_FAILED(m_ext.xrDestroyBodyTrackerFB_(m_tracker))) {
      PLOG_ERROR << "xrDestroyBodyTrackerFB_";
    }
  }

  std::span<const XrBodyJointLocationFB> Update(XrTime time, XrSpace space)
  {
    m_locations = XrBodyJointLocationsFB{
      .type = XR_TYPE_BODY_JOINT_LOCATIONS_FB,
      .next = nullptr,
      .jointCount = XR_BODY_JOINT_COUNT_FB,
      .jointLocations = m_jointLocations,
    };

    XrBodyJointsLocateInfoFB locateInfo{
      .type = XR_TYPE_BODY_JOINTS_LOCATE_INFO_FB,
      .baseSpace = space,
      .time = time,
    };
    if (XR_FAILED(
          m_ext.xrLocateBodyJointsFB_(m_tracker, &locateInfo, &m_locations))) {
      PLOG_ERROR << "xrLocateBodyJointsFB_";
      return {};
    }

    if (!m_locations.isActive) {
      return {};
    }

    return std::span{ m_jointLocations, XR_BODY_JOINT_COUNT_FB };
  }
};
