#pragma once

#include <Arduino.h>

#include "servo_axis.h"

class TrackerController {
 public:
  TrackerController(ServoAxis& yaw, ServoAxis& pitch);

  void begin();
  void arm();
  void disarm();
  void setInterlockClosed(bool closed);
  bool interlockClosed() const { return interlockClosed_; }
  bool isArmed() const { return armed_; }
  bool pwmReady() const { return yaw_.isReady() && pitch_.isReady(); }
  void updateTarget(float horizontalError, float verticalError,
                    uint32_t timestampMs);
  void stopTracking();
  void center();
  void tick(uint32_t nowMs);
  bool hasTarget(uint32_t nowMs) const;

 private:
  ServoAxis& yaw_;
  ServoAxis& pitch_;
  float horizontalError_ = 0.0F;
  float verticalError_ = 0.0F;
  uint32_t lastTargetMs_ = 0;
  uint32_t lastControlMs_ = 0;
  bool tracking_ = false;
  bool armed_ = false;
  bool interlockClosed_ = false;
};

