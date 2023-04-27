#pragma once
#include <DirectXMath.h>
#include <functional>
#include <openxr/openxr.h>
#include <plog/Log.h>
#include <span>
#include <vector>
#include <xrfw_proc.h>

struct ExtHandTracking
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_EXT_HAND_TRACKING_EXTENSION_NAME,
    };
    return s_extensions;
  }
  XRFW_PROC(xrCreateHandTrackerEXT);
  XRFW_PROC(xrDestroyHandTrackerEXT);
  XRFW_PROC(xrLocateHandJointsEXT);
  ExtHandTracking(XrInstance instance, XrSystemId system)
    : xrCreateHandTrackerEXT(instance)
    , xrDestroyHandTrackerEXT(instance)
    , xrLocateHandJointsEXT(instance)
  {
  }
};

struct ExtHandTracker
{
  const ExtHandTracking& m_ext;
  XrHandTrackerEXT m_tracker = XR_NULL_HANDLE;
  XrHandJointLocationEXT m_jointLocations[XR_HAND_JOINT_COUNT_EXT];
  XrHandJointLocationsEXT m_locations = {};

  ExtHandTracker(const ExtHandTracking& ext, XrSession session, bool isLeft)
    : m_ext(ext)
  {
    XrHandTrackerCreateInfoEXT createInfo{
      .type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
      .hand = isLeft ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT,
      .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
    };
    if (XR_FAILED(
          m_ext.xrCreateHandTrackerEXT(session, &createInfo, &m_tracker))) {
      PLOG_ERROR << "xrCreateHandTrackerEXT";
    }
  }

  ~ExtHandTracker()
  {
    if (XR_FAILED(m_ext.xrDestroyHandTrackerEXT(m_tracker))) {
      PLOG_ERROR << "xrDestroyHandTrackerEXT_";
    }
  }

  std::span<const XrHandJointLocationEXT> Update(XrTime time, XrSpace space)
  {
    m_locations = XrHandJointLocationsEXT{
      .type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
      .next = nullptr,
      .jointCount = XR_HAND_JOINT_COUNT_EXT,
      .jointLocations = m_jointLocations,
    };

    XrHandJointsLocateInfoEXT locateInfo{
      .type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
      .baseSpace = space,
      .time = time,
    };
    if (XR_FAILED(
          m_ext.xrLocateHandJointsEXT(m_tracker, &locateInfo, &m_locations))) {
      PLOG_ERROR << "xrLocateHandJointsEXT";
      return {};
    }

    if (!m_locations.isActive) {
      return {};
    }

    return std::span{ m_jointLocations, XR_HAND_JOINT_COUNT_EXT };
  }
};
