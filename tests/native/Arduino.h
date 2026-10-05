#pragma once
// Host-only shim: no real GPIO, timing, or motors are exercised by these tests.
#include <algorithm>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>
using std::min;
inline uint32_t fakeNow = 0;
inline uint32_t duty[16] = {};
constexpr int LOW = 0, HIGH = 1, OUTPUT = 2, INPUT_PULLUP = 3;
inline int gpio[64] = {};
struct OutputEvent { char kind; uint8_t index; uint32_t value; };
inline std::vector<OutputEvent> outputEvents;
inline uint32_t setupResult[16] = {}; // Zero means no override, return requested frequency.
inline bool setupFails[16] = {};
inline uint32_t setupFrequency[16] = {};
inline uint8_t setupBits[16] = {};
inline void digitalWrite(uint8_t pin, int value) {
  gpio[pin] = value; outputEvents.push_back({'G',pin,static_cast<uint32_t>(value)});
}
inline int digitalRead(uint8_t pin) { return gpio[pin]; }
inline void pinMode(uint8_t, int) {}
inline uint32_t millis() { return fakeNow; }
inline void delay(uint32_t duration) { fakeNow += duration; }
inline uint32_t ledcSetup(uint8_t channel, uint32_t frequency, uint8_t bits) {
  setupFrequency[channel]=frequency; setupBits[channel]=bits;
  outputEvents.push_back({'S',channel,bits});
  // Model S3 capability; the old 16-bit configuration must fail this shim too.
  if (channel>=8 || bits>14 || setupFails[channel]) return 0;
  return setupResult[channel] ? setupResult[channel] : frequency;
}
inline void ledcAttachPin(uint8_t pin, uint8_t channel) {
  outputEvents.push_back({'A',channel,pin});
}
inline void ledcWrite(uint8_t channel, uint32_t value) {
  duty[channel] = value; outputEvents.push_back({'W',channel,value});
}
struct FakeSerial {
  std::deque<uint8_t> input;
  std::string output;
  void begin(unsigned) {}
  int available() const { return static_cast<int>(input.size()); }
  int read() { const auto c=input.front(); input.pop_front(); return c; }
  void println(const char* text="") { output += text; output += '\n'; }
  void printf(const char* format, ...) {
    char buffer[512]; va_list args; va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args); va_end(args); output += buffer;
  }
  void feed(const char* text) { while (*text) input.push_back(static_cast<uint8_t>(*text++)); }
};
inline FakeSerial Serial;
template <typename T> T constrain(T value, T low, T high) {
  return std::max(low, std::min(value, high));
}
