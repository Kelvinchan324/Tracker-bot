#pragma once
// Host-only shim: no real GPIO, timing, or motors are exercised by these tests.
#include <algorithm>
#include <cstdint>
using std::min;
inline uint32_t fakeNow = 0;
inline uint32_t duty[16] = {};
inline uint32_t millis() { return fakeNow; }
inline void ledcSetup(uint8_t, uint16_t, uint8_t) {}
inline void ledcAttachPin(uint8_t, uint8_t) {}
inline void ledcWrite(uint8_t channel, uint32_t value) { duty[channel] = value; }
template <typename T> T constrain(T value, T low, T high) {
  return std::max(low, std::min(value, high));
}
