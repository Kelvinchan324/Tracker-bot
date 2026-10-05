#pragma once
#include <stdint.h>
enum class CameraFrameState { NoFrame, InvalidFrame, StaleFrame, NoMarker, Marker };
struct CameraTarget {
  bool found;
  float x;
  float y;
  uint32_t timestamp;  // Capture-start milliseconds, not retrieval/processing time.
  uint32_t pixels;     // Red samples, not full-resolution pixel area.
  CameraFrameState state;
};
bool startCameraTarget();
bool readCameraTarget(CameraTarget& target);
