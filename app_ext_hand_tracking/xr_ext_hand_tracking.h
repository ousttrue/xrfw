#pragma once
#include <DirectXMath.h>
#include <functional>
#include <openxr/openxr.h>
#include <plog/Log.h>
#include <span>
#include <vector>

struct ExtHandTracking
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_EXT_HAND_TRACKING_EXTENSION_NAME,
    };
    return s_extensions;
  }
  PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_ = nullptr;
  PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_ = nullptr;
  PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_ = nullptr;
  ExtHandTracking(XrInstance instance, XrSystemId system)
  {
    // Inspect hand tracking system properties
    XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{
      XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT
    };
    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES,
                                         &handTrackingSystemProperties };
    if (XR_FAILED(xrGetSystemProperties(instance, system, &systemProperties))) {
      PLOG_ERROR << "xrGetSystemProperties";
    }
    if (!handTrackingSystemProperties.supportsHandTracking) {
      // The system does not support hand tracking
      PLOG_ERROR << "xrGetSystemProperties "
                    "XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT FAILED.";
    } else {
      PLOG_INFO << "xrGetSystemProperties "
                   "XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT OK "
                   "- initiallizing hand tracking...";
    }

    if (XR_FAILED(xrGetInstanceProcAddr(
          instance,
          "xrCreateHandTrackerEXT",
          (PFN_xrVoidFunction*)(&xrCreateHandTrackerEXT_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrCreateHandTrackerEXT";
    }
    if (XR_FAILED(xrGetInstanceProcAddr(
          instance,
          "xrDestroyHandTrackerEXT",
          (PFN_xrVoidFunction*)(&xrDestroyHandTrackerEXT_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrDestroyHandTrackerEXT_";
    }
    if (XR_FAILED(xrGetInstanceProcAddr(
          instance,
          "xrLocateHandJointsEXT",
          (PFN_xrVoidFunction*)(&xrLocateHandJointsEXT_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrLocateHandJointsEXT_";
    }
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
    if (XR_FAILED(m_ext.xrCreateHandTrackerEXT_(
          session, &createInfo, &m_tracker))) {
      PLOG_ERROR << "xrCreateHandTrackerEXT";
    }
  }

  ~ExtHandTracker()
  {
    if (XR_FAILED(m_ext.xrDestroyHandTrackerEXT_(m_tracker))) {
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
    if (XR_FAILED(m_ext.xrLocateHandJointsEXT_(
          m_tracker, &locateInfo, &m_locations))) {
      PLOG_ERROR << "xrLocateHandJointsEXT";
      return {};
    }

    if (!m_locations.isActive) {
      return {};
    }

    return std::span{ m_jointLocations, XR_HAND_JOINT_COUNT_EXT };
  }
};
