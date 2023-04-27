#pragma once
#include <DirectXMath.h>
#include <functional>
#include <openxr/openxr.h>
#include <plog/Log.h>
#include <span>
#include <vector>
#include <xrfw_proc.h>

struct FbBodyTracking
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_FB_BODY_TRACKING_EXTENSION_NAME,
    };
    return s_extensions;
  }
  XRFW_PROC(xrCreateBodyTrackerFB);
  XRFW_PROC(xrDestroyBodyTrackerFB);
  XRFW_PROC(xrLocateBodyJointsFB);
  XRFW_PROC(xrGetBodySkeletonFB);

  FbBodyTracking(XrInstance instance, XrSystemId system)
    : xrCreateBodyTrackerFB(instance)
    , xrDestroyBodyTrackerFB(instance)
    , xrLocateBodyJointsFB(instance)
    , xrGetBodySkeletonFB(instance)
  {
  }
};

struct FbBodyTracker
{
  const FbBodyTracking& m_ext;
  XrBodyTrackerFB m_tracker = XR_NULL_HANDLE;
  XrBodyJointLocationFB m_jointLocations[XR_BODY_JOINT_COUNT_FB];
  XrBodyJointLocationsFB m_locations = {};

  uint32_t m_skeletonChangeCount = -1;
  XrBodySkeletonJointFB m_skeletonJoints[XR_BODY_JOINT_COUNT_FB];

  FbBodyTracker(const FbBodyTracking& ext, XrSession session)
    : m_ext(ext)
  {
    XrBodyTrackerCreateInfoFB createInfo{
      .type = XR_TYPE_BODY_TRACKER_CREATE_INFO_FB,
      .bodyJointSet = XR_BODY_JOINT_SET_DEFAULT_FB,
    };
    if (XR_FAILED(
          m_ext.xrCreateBodyTrackerFB(session, &createInfo, &m_tracker))) {
      PLOG_ERROR << "xrCreateBodyTrackerFB_";
    }
  }

  ~FbBodyTracker()
  {
    if (XR_FAILED(m_ext.xrDestroyBodyTrackerFB(m_tracker))) {
      PLOG_ERROR << "xrDestroyBodyTrackerFB_";
    }
  }

  std::span<const XrBodyJointLocationFB> Joints() const
  {
    return { m_jointLocations, m_jointLocations + XR_BODY_JOINT_COUNT_FB };
  }
  std::span<const XrBodySkeletonJointFB> SkeletonJoints() const
  {
    return { m_skeletonJoints, m_skeletonJoints + XR_BODY_JOINT_COUNT_FB };
  }

  struct Result
  {
    bool SkeletonUpdated;
    bool JointsIsActive;
  };

  Result Update(XrTime time, XrSpace space)
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
          m_ext.xrLocateBodyJointsFB(m_tracker, &locateInfo, &m_locations))) {
      PLOG_ERROR << "xrLocateBodyJointsFB_";
      return {};
    }

    if (!m_locations.isActive) {
      return {};
    }

    Result result{
      .SkeletonUpdated = false,
      .JointsIsActive = true,
    };
    if (m_locations.skeletonChangedCount != m_skeletonChangeCount) {
      m_skeletonChangeCount = m_locations.skeletonChangedCount;

      // retrieve the updated skeleton
      XrBodySkeletonFB skeleton{
        .type = XR_TYPE_BODY_SKELETON_FB,
        .next = nullptr,
        .jointCount = XR_BODY_JOINT_COUNT_FB,
        .joints = m_skeletonJoints,
      };

      if (XR_FAILED(m_ext.xrGetBodySkeletonFB(m_tracker, &skeleton))) {
        PLOG_ERROR << "xrGetSkeletonFB_";
      }

      result.SkeletonUpdated = true;
    }

    return result;
  }
};
