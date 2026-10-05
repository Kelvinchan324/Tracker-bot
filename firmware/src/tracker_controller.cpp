#include "tracker_controller.h"

#include <math.h>

#include "app_config.h"

namespace {

float applyDeadband(float value) {
  return fabsf(value) < config::kTargetDeadband ? 0.0F : value;
}

}  // namespace

TrackerController::TrackerController(ServoAxis& yaw, ServoAxis& pitch)
    : yaw_(yaw), pitch_(pitch) {}

void TrackerController::begin() {
  digitalWrite(config::kPwmEnablePin, LOW);
  pinMode(config::kPwmEnablePin, OUTPUT);
  stopTracking();
  armed_ = false;
  interlockClosed_ = false;
  yaw_.begin();
  pitch_.begin();
  lastControlMs_ = millis();
}

void TrackerController::updateTarget(float horizontalError,
                                     float verticalError,
                                     uint32_t timestampMs) {
  if (!armed_ || !isfinite(horizontalError) || !isfinite(verticalError) ||
      fabsf(horizontalError) > 1.0F || fabsf(verticalError) > 1.0F ||
      uint32_t(millis() - timestampMs) > config::kTargetTimeoutMs) {
    stopTracking();
    return;
  }
  horizontalError_ = constrain(horizontalError, -1.0F, 1.0F);
  verticalError_ = constrain(verticalError, -1.0F, 1.0F);
  lastTargetMs_ = timestampMs;
  tracking_ = true;
}

void TrackerController::arm() {
  if (!interlockClosed_ || !pwmReady()) return;
  stopTracking();
  lastControlMs_ = millis();
  yaw_.enable();
  pitch_.enable();
  digitalWrite(config::kPwmEnablePin, HIGH);
  armed_ = true;
}

void TrackerController::disarm() {
  digitalWrite(config::kPwmEnablePin, LOW);
  stopTracking();
  armed_ = false;
  yaw_.disable();
  pitch_.disable();
}

void TrackerController::setInterlockClosed(bool closed) {
  interlockClosed_ = closed;
  if (!closed) disarm();
}

void TrackerController::stopTracking() {
  tracking_ = false;
  horizontalError_ = 0.0F;
  verticalError_ = 0.0F;
}

void TrackerController::center() {
  if (!armed_) return;
  stopTracking();
  yaw_.center();
  pitch_.center();
}

bool TrackerController::hasTarget(uint32_t nowMs) const {
  return armed_ && tracking_ && (nowMs - lastTargetMs_ <= config::kTargetTimeoutMs);
}

void TrackerController::tick(uint32_t nowMs) {
  if (nowMs - lastControlMs_ < config::kControlPeriodMs) {
    return;
  }

  // Never turn a stalled event loop into a large catch-up movement.
  const float elapsedSeconds = min(nowMs - lastControlMs_, uint32_t(40)) / 1000.0F;
  lastControlMs_ = nowMs;

  if (!hasTarget(nowMs)) {
    stopTracking();
    return;
  }

  const float yawError = applyDeadband(horizontalError_);
  const float pitchError = applyDeadband(verticalError_);

  // Reverse either sign here if an axis moves away from the target.
  yaw_.moveBy(yawError * config::kYawGainDegPerSecond * elapsedSeconds);
  pitch_.moveBy(pitchError * config::kPitchGainDegPerSecond * elapsedSeconds);
}

