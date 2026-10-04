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
  yaw_.begin();
  pitch_.begin();
  lastControlMs_ = millis();
}

void TrackerController::updateTarget(float horizontalError,
                                     float verticalError,
                                     uint32_t timestampMs) {
  horizontalError_ = constrain(horizontalError, -1.0F, 1.0F);
  verticalError_ = constrain(verticalError, -1.0F, 1.0F);
  lastTargetMs_ = timestampMs;
  tracking_ = true;
}

void TrackerController::stopTracking() {
  tracking_ = false;
  horizontalError_ = 0.0F;
  verticalError_ = 0.0F;
}

void TrackerController::center() {
  stopTracking();
  yaw_.center();
  pitch_.center();
}

bool TrackerController::hasTarget(uint32_t nowMs) const {
  return tracking_ && (nowMs - lastTargetMs_ <= config::kTargetTimeoutMs);
}

void TrackerController::tick(uint32_t nowMs) {
  if (nowMs - lastControlMs_ < config::kControlPeriodMs) {
    return;
  }

  const float elapsedSeconds = (nowMs - lastControlMs_) / 1000.0F;
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

