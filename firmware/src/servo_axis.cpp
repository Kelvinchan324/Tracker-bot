#include "servo_axis.h"

#include "app_config.h"
#include <math.h>

namespace {

uint32_t pulseToDuty(float pulseUs) {
  constexpr uint32_t kPeriodUs =
      1000000UL / config::kServoPwmFrequencyHz;
  constexpr uint32_t kCountsPerPeriod = 1UL << config::kServoPwmResolutionBits;
  return static_cast<uint32_t>(lroundf(pulseUs * kCountsPerPeriod / kPeriodUs));
}

}  // namespace

ServoAxis::ServoAxis(uint8_t pin, uint8_t channel, float minimumDeg,
                     float centerDeg, float maximumDeg)
    : pin_(pin),
      channel_(channel),
      minimumDeg_(minimumDeg),
      centerDeg_(centerDeg),
      maximumDeg_(maximumDeg),
      angleDeg_(centerDeg) {}

void ServoAxis::begin() {
  disable();
  ready_ = false;
  const uint32_t actualHz = ledcSetup(channel_, config::kServoPwmFrequencyHz,
                                    config::kServoPwmResolutionBits);
  if (actualHz != config::kServoPwmFrequencyHz) return;
  ready_ = true;
  // Attach inherits channel duty in pinned Arduino 2.0.17; zero it first.
  ledcWrite(channel_, 0);
  ledcAttachPin(pin_, channel_);
}

void ServoAxis::enable() {
  if (!ready_) return;
  enabled_ = true; setAngle(angleDeg_);
}
void ServoAxis::disable() {
  enabled_ = false;
  if (ready_) ledcWrite(channel_, 0);
}

void ServoAxis::setAngle(float angleDeg) {
  if (!enabled_ || !isfinite(angleDeg)) return;
  angleDeg_ = constrain(angleDeg, minimumDeg_, maximumDeg_);
  const float fraction = angleDeg_ / 180.0F;
  const float pulseUs =
      config::kServoMinPulseUs +
      fraction * (config::kServoMaxPulseUs - config::kServoMinPulseUs);
  ledcWrite(channel_, pulseToDuty(pulseUs));
}

void ServoAxis::moveBy(float deltaDeg) { setAngle(angleDeg_ + deltaDeg); }

void ServoAxis::center() { setAngle(centerDeg_); }

float ServoAxis::angle() const { return angleDeg_; }

