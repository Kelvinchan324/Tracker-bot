#pragma once

#include <Arduino.h>

class ServoAxis {
 public:
  ServoAxis(uint8_t pin, uint8_t channel, float minimumDeg, float centerDeg,
            float maximumDeg);

  void begin();
  void enable();
  void disable();
  void setAngle(float angleDeg);
  void moveBy(float deltaDeg);
  void center();
  float angle() const;
  bool isReady() const { return ready_; }

 private:
  uint8_t pin_;
  uint8_t channel_;
  float minimumDeg_;
  float centerDeg_;
  float maximumDeg_;
  float angleDeg_;
  bool enabled_ = false;
  bool ready_ = false;
};

