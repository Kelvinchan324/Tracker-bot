#include "servo_axis.h"

#include "app_config.h"

namespace {

uint32_t pulseToDuty(uint16_t pulseUs) {
  constexpr uint32_t kPeriodUs =
      1000000UL / config::kServoPwmFrequencyHz;
  constexpr uint32_t kMaximumDuty =
      (1UL << config::kServoPwmResolutionBits) - 1UL;
  return (static_cast<uint32_t>(pulseUs) * kMaximumDuty) / kPeriodUs;
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
  ledcSetup(channel_, config::kServoPwmFrequencyHz,
            config::kServoPwmResolutionBits);
  ledcAttachPin(pin_, channel_);
  center();
}

void ServoAxis::setAngle(float angleDeg) {
  angleDeg_ = constrain(angleDeg, minimumDeg_, maximumDeg_);
  const float fraction = angleDeg_ / 180.0F;
  const uint16_t pulseUs = static_cast<uint16_t>(
      config::kServoMinPulseUs +
      fraction * (config::kServoMaxPulseUs - config::kServoMinPulseUs));
  ledcWrite(channel_, pulseToDuty(pulseUs));
}

void ServoAxis::moveBy(float deltaDeg) { setAngle(angleDeg_ + deltaDeg); }

void ServoAxis::center() { setAngle(centerDeg_); }

float ServoAxis::angle() const { return angleDeg_; }

