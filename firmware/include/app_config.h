#pragma once

#include <Arduino.h>

namespace config {

// Change these pins before connecting hardware. Avoid pins used by the camera,
// flash, PSRAM, or USB on your final ESP32-S3 board.
#ifdef TRACKER_XIAO_SENSE
constexpr uint8_t kYawServoPin = 1;    // D0; camera pins are separate.
constexpr uint8_t kPitchServoPin = 2;  // D1.
#else
constexpr uint8_t kYawServoPin = 4;
constexpr uint8_t kPitchServoPin = 5;
#endif

constexpr uint8_t kYawPwmChannel = 0;
constexpr uint8_t kPitchPwmChannel = 1;
constexpr uint16_t kServoPwmFrequencyHz = 50;
constexpr uint8_t kServoPwmResolutionBits = 16;

// Start with conservative limits and widen them only after checking the CAD.
constexpr float kYawMinDeg = 60.0F;
constexpr float kYawCenterDeg = 90.0F;
constexpr float kYawMaxDeg = 120.0F;
constexpr float kPitchMinDeg = 70.0F;
constexpr float kPitchCenterDeg = 90.0F;
constexpr float kPitchMaxDeg = 110.0F;

// Typical hobby-servo pulse range. Confirm it against the selected servo.
constexpr uint16_t kServoMinPulseUs = 1000;
constexpr uint16_t kServoMaxPulseUs = 2000;

// Target coordinates are normalized: -1 is left/up and +1 is right/down.
constexpr float kTargetDeadband = 0.08F;
constexpr float kYawGainDegPerSecond = 55.0F;
constexpr float kPitchGainDegPerSecond = 45.0F;
constexpr uint32_t kTargetTimeoutMs = 750;
constexpr uint32_t kControlPeriodMs = 20;

}  // namespace config

