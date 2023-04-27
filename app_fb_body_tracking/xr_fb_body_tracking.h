#pragma once
#include <DirectXMath.h>
#include <functional>
#include <openxr/openxr.h>
#include <plog/Log.h>
#include <span>
#include <vector>

// https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
template<size_t N>
struct XrFwStringLiteral
{
  constexpr XrFwStringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }
  char value[N];
};

template<typename PFN, XrFwStringLiteral NAME>
struct XrFwInstanceProc
{
  PFN Proc = nullptr;
  XrFwInstanceProc(XrInstance instance)
  {
    if (XR_FAILED(xrGetInstanceProcAddr(
          instance, NAME.value, (PFN_xrVoidFunction*)&Proc))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: " << NAME.value;
    }
  }
};

struct FbBodyTracking
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_FB_BODY_TRACKING_EXTENSION_NAME,
    };
    return s_extensions;
  }
  XrFwInstanceProc<PFN_xrCreateBodyTrackerFB, "xrCreateBodyTrackerFB">
    xrCreateBodyTrackerFB;
  PFN_xrDestroyBodyTrackerFB xrDestroyBodyTrackerFB_ = nullptr;
  PFN_xrLocateBodyJointsFB xrLocateBodyJointsFB_ = nullptr;
  // PFN_xrGetBodySkeletonFB xrGetSkeletonFB_ = nullptr;

  FbBodyTracking(XrInstance instance, XrSystemId system)
    : xrCreateBodyTrackerFB(instance)
  {
    // if (XR_FAILED(xrGetInstanceProcAddr(
    //       instance,
    //       "xrCreateBodyTrackerFB",
    //       (PFN_xrVoidFunction*)(&xrCreateBodyTrackerFB_)))) {
    //   PLOG_ERROR << "xrGetInstanceProcAddr: xrCreateBodyTrackerFB";
    // }
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

  uint32_t m_skeletonChangeCount = 0;
  XrBodySkeletonJointFB m_skeletonJoints[XR_BODY_JOINT_COUNT_FB];

  FbBodyTracker(const FbBodyTracking& ext, XrSession session)
    : m_ext(ext)
  {
    XrBodyTrackerCreateInfoFB createInfo{
      .type = XR_TYPE_BODY_TRACKER_CREATE_INFO_FB,
      .bodyJointSet = XR_BODY_JOINT_SET_DEFAULT_FB,
    };
    if (XR_FAILED(
          m_ext.xrCreateBodyTrackerFB.Proc(session, &createInfo, &m_tracker))) {
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

    // if (m_locations.skeletonChangedCount != skeletonChangeCount_) {
    //   m_skeletonChangeCount = m_locations.skeletonChangedCount;
    //
    //   // retrieve the updated skeleton
    //   XrBodySkeletonFB skeleton{
    //     .type = XR_TYPE_BODY_SKELETON_FB,
    //     .next = nullptr,
    //     .jointCount = XR_BODY_JOINT_COUNT_FB,
    //     .joints = m_skeletonJoints,
    //   };
    //
    //   if (XR_FAILED(m_ext.xrGetSkeletonFB_(m_bodyTracker, &skeleton))) {
    //     PLOG_ERROR << "xrGetSkeletonFB_";
    //   }
    // }

    return std::span{ m_jointLocations, XR_BODY_JOINT_COUNT_FB };
  }
};
