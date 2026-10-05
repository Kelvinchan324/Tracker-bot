#pragma once
#include "camera_target.h"
#include "red_target.h"

// Pure input boundary, shared by the capture task and native tests. The reference
// configuration is exactly 160x120 RGB565, tightly packed and MSB-first.
inline CameraTarget processCameraFrame(const uint8_t* data, size_t length,
    size_t width, size_t height, bool rgb565, int64_t seconds, int64_t microseconds,
    int64_t nowUs, uint32_t maxAgeMs, RedTargetWorkspace& work) {
  CameraTarget target{false, 0, 0, 0, 0, CameraFrameState::InvalidFrame, 0, 0};
  if (!rgb565 || !data || width != 160 || height != 120 || length != 160 * 120 * 2 ||
      seconds < 0 || microseconds < 0 || microseconds >= 1000000 || nowUs < 0)
    return target;
  // Check seconds before multiplying; malformed large values cannot overflow.
  if (seconds > nowUs / 1000000) return target;
  const uint64_t captureUs = uint64_t(seconds) * 1000000 + uint64_t(microseconds);
  if (captureUs > uint64_t(nowUs)) return target;
  target.timestamp = uint32_t(captureUs / 1000);  // Same rollover domain as millis().
  if (uint64_t(nowUs) - captureUs > uint64_t(maxAgeMs) * 1000) {
    target.state = CameraFrameState::StaleFrame;
    return target;
  }
  const auto result = findRedTarget(data, length, uint16_t(width), uint16_t(height), work);
  target.found = result.found;
  target.x = result.x;
  target.y = result.y;
  target.pixels = result.pixels;
  target.candidates = result.candidates;
  target.selectedPixels = result.selectedPixels;
  target.state = result.candidates > 1 ? CameraFrameState::Ambiguous :
      result.found ? CameraFrameState::Marker : CameraFrameState::NoMarker;
  return target;
}

inline const char* cameraFrameStateName(CameraFrameState state) {
  switch (state) {
    case CameraFrameState::InvalidFrame: return "invalid-frame";
    case CameraFrameState::StaleFrame: return "stale-frame";
    case CameraFrameState::NoMarker: return "no-marker";
    case CameraFrameState::Marker: return "marker";
    case CameraFrameState::Ambiguous: return "ambiguous";
    default: return "no-frame";
  }
}
