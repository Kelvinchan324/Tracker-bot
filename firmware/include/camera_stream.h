#pragma once
#include <stdint.h>
#include "camera_target.h"

// Main-loop freshness gate, not an independent task/hardware watchdog. Capture
// timestamps, never receipt times, determine eligibility across uint32 rollover.
class CameraStreamMonitor {
 public:
  explicit CameraStreamMonitor(uint32_t timeoutMs) : timeoutMs_(timeoutMs) {}
  void observe(const CameraTarget& frame, uint32_t nowMs) {
    seen_ = true;
    captureMs_ = frame.timestamp;
    if (frame.state == CameraFrameState::NoFrame) state_ = "no-frame";
    else if (frame.state == CameraFrameState::InvalidFrame) state_ = "invalid-frame";
    else if (frame.state == CameraFrameState::StaleFrame) state_ = "stale-frame";
    else if (frame.state != CameraFrameState::Marker &&
             frame.state != CameraFrameState::NoMarker &&
             frame.state != CameraFrameState::Ambiguous) state_ = "invalid-frame";
    else if (uint32_t(nowMs - captureMs_) > timeoutMs_) state_ = "stale-frame";
    else { valid_ = true; state_ = "live"; return; }
    valid_ = false;
  }
  bool usable(uint32_t nowMs) const {
    return valid_ && uint32_t(nowMs - captureMs_) <= timeoutMs_;
  }
  const char* state(uint32_t nowMs) const {
    if (!seen_) return "waiting";
    if (valid_ && !usable(nowMs)) return "timed-out";
    return state_;
  }
 private:
  uint32_t timeoutMs_, captureMs_ = 0;
  bool seen_ = false, valid_ = false;
  const char* state_ = "waiting";
};
