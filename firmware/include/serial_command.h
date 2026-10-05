#pragma once
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tracker_controller.h"

enum class CommandKind { Empty, Invalid, Arm, Disarm, Camera, Serial, Center,
                         Stop, Status, Help, Target };
struct SerialCommand { CommandKind kind; float x; float y; };

inline bool commandEquals(const char* word, const char* expected) {
  while (*word && *expected) {
    const char c = *word >= 'A' && *word <= 'Z' ? *word + ('a' - 'A') : *word;
    if (c != *expected) return false;
    ++word; ++expected;
  }
  return *word == *expected;
}
inline char* commandToken(char*& cursor) {
  while (*cursor == ' ' || *cursor == '\t') ++cursor;
  if (!*cursor) return nullptr;
  char* start = cursor;
  while (*cursor && *cursor != ' ' && *cursor != '\t') ++cursor;
  if (*cursor) *cursor++ = '\0';
  return start;
}
inline bool targetNumber(const char* word, float& out) {
  if (!word) return false;
  const char* p = word;
  if (*p == '+' || *p == '-') ++p;
  unsigned digits = 0;
  while (*p >= '0' && *p <= '9') { ++digits; ++p; }
  if (*p == '.') {
    ++p;
    while (*p >= '0' && *p <= '9') { ++digits; ++p; }
  }
  if (!digits) return false;
  if (*p == 'e' || *p == 'E') {
    ++p;
    if (*p == '+' || *p == '-') ++p;
    unsigned exponentDigits = 0;
    while (*p >= '0' && *p <= '9') { ++exponentDigits; ++p; }
    if (!exponentDigits) return false;
  }
  if (*p) return false; // No NaN, infinity, hexadecimal, suffixes or comma decimals.
  char* end = nullptr;
  errno = 0;
  const double value = strtod(word, &end);
  if (errno == ERANGE || !end || *end || !isfinite(value) || fabs(value) > 1.0)
    return false;
  out = static_cast<float>(value);
  return true;
}
inline SerialCommand parseSerialCommand(const char* line) {
  SerialCommand result{CommandKind::Invalid, 0, 0};
  if (!line) return result;
  char copy[97];
  size_t length = 0;
  while (line[length]) {
    const unsigned char c = static_cast<unsigned char>(line[length]);
    if (length == 96 || (c != '\t' && (c < 32 || c > 126))) return result;
    copy[length] = line[length]; ++length;
  }
  copy[length] = '\0';
  char* cursor = copy;
  char* command = commandToken(cursor);
  if (!command) return {CommandKind::Empty, 0, 0};
  char* first = commandToken(cursor);
  char* second = commandToken(cursor);
  if (commandToken(cursor)) return result;
  if (commandEquals(command, "target")) {
    if (targetNumber(first, result.x) && targetNumber(second, result.y))
      result.kind = CommandKind::Target;
    return result;
  }
  if (first || second) return result;
  const char* names[] = {"arm", "disarm", "camera", "serial", "center", "stop", "status", "help"};
  const CommandKind kinds[] = {CommandKind::Arm, CommandKind::Disarm, CommandKind::Camera,
    CommandKind::Serial, CommandKind::Center, CommandKind::Stop, CommandKind::Status, CommandKind::Help};
  for (size_t i = 0; i < 8; ++i) if (commandEquals(command, names[i])) result.kind = kinds[i];
  return result;
}

// Shared production command effects; diagnostics are emitted by main.cpp.
inline const char* applySerialCommand(const SerialCommand& command, TrackerController& tracker,
    bool& cameraMode, bool cameraReady, bool stopClosed, uint32_t nowMs) {
  tracker.setInterlockClosed(stopClosed);
  if (!stopClosed) cameraMode = false;
  switch (command.kind) {
    case CommandKind::Empty: case CommandKind::Status: case CommandKind::Help: return nullptr;
    case CommandKind::Arm:
      tracker.arm();
      return tracker.isArmed() ? "Armed: last commanded position applied" : "Blocked: stop contact open";
    case CommandKind::Disarm:
      cameraMode = false; tracker.disarm(); return "Disarmed: PWM disabled";
    case CommandKind::Camera:
      tracker.stopTracking(); cameraMode = cameraReady;
      return cameraMode ? "Red marker camera mode" : "Camera unavailable";
    case CommandKind::Serial:
      cameraMode = false; tracker.stopTracking(); return "Serial target mode";
    case CommandKind::Center:
      cameraMode = false; tracker.center();
      return tracker.isArmed() ? "Center commanded; serial mode" : "Ignored: disarmed";
    case CommandKind::Stop:
      cameraMode = false; tracker.stopTracking(); return "Tracking stopped; PWM held";
    case CommandKind::Target:
      if (tracker.isArmed() && !cameraMode) {
        tracker.updateTarget(command.x, command.y, nowMs);
        return "Target updated";
      }
      cameraMode = false; tracker.stopTracking();
      return "Target rejected: requires armed serial mode; tracking stopped";
    default:
      cameraMode = false; tracker.stopTracking();
      return "Invalid command: tracking stopped; PWM held. Type help.";
  }
}

enum class LineEvent { None, Complete, Rejected };
class SerialLineBuffer {
 public:
  static constexpr uint32_t kLineTimeoutMs = 1000;
  bool expire(uint32_t nowMs) {
    if (active_ && !discard_ && uint32_t(nowMs - startedMs_) > kLineTimeoutMs) {
      discard_ = true; length_ = 0; return true;
    }
    return false;
  }
  LineEvent push(uint8_t c, uint32_t nowMs) {
    if (expire(nowMs)) {
      if (c == '\r' || c == '\n') reset();
      return LineEvent::Rejected;
    }
    if (discard_) {
      if (c == '\r' || c == '\n') reset();
      return LineEvent::None;
    }
    if (c == '\r' || c == '\n') {
      if (!length_) { reset(); return LineEvent::None; }
      buffer_[length_] = '\0'; length_ = 0; active_ = false;
      return LineEvent::Complete;
    }
    if (length_ == 96 || (c != '\t' && (c < 32 || c > 126))) {
      discard_ = true; length_ = 0; return LineEvent::Rejected;
    }
    if (!active_) { active_ = true; startedMs_ = nowMs; }
    buffer_[length_++] = static_cast<char>(c);
    return LineEvent::None;
  }
  const char* line() const { return buffer_; } // Consume immediately after Complete.
 private:
  void reset() { length_ = 0; active_ = false; discard_ = false; buffer_[0] = '\0'; }
  char buffer_[97]{};
  size_t length_ = 0;
  bool active_ = false, discard_ = false;
  uint32_t startedMs_ = 0;
};
