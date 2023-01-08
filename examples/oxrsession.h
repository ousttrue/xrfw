#pragma once
#include <openxr/openxr.h>

class OxrSessionState {
  XrInstance instance_ = nullptr;
  XrSession session_ = nullptr;

  XrEventDataBuffer eventDataBuffer_ = {};
  XrSessionState sessionState_ = XR_SESSION_STATE_UNKNOWN;
  bool sessionRunning_ = false;

public:
  OxrSessionState(XrInstance instance, XrSession session);
  ~OxrSessionState();

  bool PollEventsAndIsActive();

private:
  const XrEventDataBaseHeader *TryReadNextEvent();
  void HandleSessionStateChangedEvent(
      const XrEventDataSessionStateChanged &stateChangedEvent);
};
