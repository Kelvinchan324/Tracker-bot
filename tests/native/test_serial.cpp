#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "serial_command.h"
#include "app_config.h"

int main() {
  for (const char* text : {"target 0 0", " TARGET\t+.2\t-1e-1 ", "target -1 1",
                           "target .5 1.", "target -0 +0"})
    assert(parseSerialCommand(text).kind == CommandKind::Target);
  const auto decimal = parseSerialCommand("target .25 -1e-1");
  assert(std::fabs(decimal.x - .25F) < .0001F && std::fabs(decimal.y + .1F) < .0001F);
  for (const char* text : {"target", "target 0", "target0 0", "target 0 0 extra",
      "target nan 0", "target 0 inf", "target 0x1p-1 0", "target 1.01 0",
      "target 1.00000001 0", "target 0 -1.01", "target 1e999 0", "target 1e-999 0",
      "target . 0", "target + 0", "target 1e 0", "target .2-.1", "target 0,2 0",
      "target 0 0x", "arm extra", "stop now", "status extra", "disarm extra", "stpo"})
    assert(parseSerialCommand(text).kind == CommandKind::Invalid);
  assert(parseSerialCommand(nullptr).kind == CommandKind::Invalid);
  assert(parseSerialCommand(std::string(97, ' ').c_str()).kind == CommandKind::Invalid);
  assert(parseSerialCommand(" \t ").kind == CommandKind::Empty);
  assert(parseSerialCommand("DISARM").kind == CommandKind::Disarm);
  assert(parseSerialCommand("status").kind == CommandKind::Status);

  ServoAxis yaw(1,0,60,90,120), pitch(2,1,70,90,110);
  TrackerController tracker(yaw,pitch);
  tracker.begin();
  bool camera = false;
  auto execute = [&](const char* text, bool contact=true, bool ready=true) {
    return applySerialCommand(parseSerialCommand(text), tracker, camera, ready, contact, fakeNow);
  };
  execute("arm", false);
  assert(!tracker.isArmed());
  execute("arm extra");
  assert(!tracker.isArmed());
  execute("ARM");
  assert(tracker.isArmed() && gpio[config::kPwmEnablePin] == HIGH);
  execute("target .5 0");
  assert(tracker.hasTarget(fakeNow));
  fakeNow = 600;
  execute("help"); execute("status"); execute(" ");
  assert(tracker.hasTarget(fakeNow)); // Read-only/empty commands do not extend or cancel target.
  assert(!tracker.hasTarget(751));
  execute("target nan 0");
  assert(!tracker.hasTarget(fakeNow) && tracker.isArmed() && !camera);
  assert(gpio[config::kPwmEnablePin] == HIGH); // Stop increments, not physical power or PWM.
  execute("camera"); assert(camera);
  execute("target .5 0"); // Wrong input mode is explicit rejection and leaves camera mode.
  assert(!camera && !tracker.hasTarget(fakeNow));
  execute("target .5 0"); assert(tracker.hasTarget(fakeNow));
  execute("stpo"); assert(!tracker.hasTarget(fakeNow));
  execute("camera", true, false); assert(!camera);
  execute("camera"); execute("serial"); assert(!camera);
  execute("center"); assert(!tracker.hasTarget(fakeNow));
  execute("target .5 0"); execute("stop"); assert(!tracker.hasTarget(fakeNow));
  execute("camera"); execute("status", false);
  assert(!tracker.isArmed() && !camera && gpio[config::kPwmEnablePin] == LOW);
  execute("status", true); assert(!tracker.isArmed()); // Contact release does not arm.
  execute("target .5 0"); assert(!tracker.isArmed() && !tracker.hasTarget(fakeNow));
  execute("arm"); execute("disarm");
  assert(!tracker.isArmed() && duty[0] == 0 && duty[1] == 0);

  SerialLineBuffer buffer;
  std::vector<std::string> completed;
  unsigned rejected = 0;
  auto feed = [&](const std::string& data, uint32_t now) {
    for (unsigned char c : data) {
      const auto event = buffer.push(c, now);
      if (event == LineEvent::Complete) completed.emplace_back(buffer.line());
      if (event == LineEvent::Rejected) ++rejected;
    }
  };
  feed("arm\r\nstatus\n", 0);
  assert(completed.size() == 2 && completed[0] == "arm" && completed[1] == "status");
  feed(std::string(96, ' ') + "\n", 1);
  assert(completed.size() == 3 && completed.back().size() == 96);
  feed(std::string(97, ' ') + "arm\n", 2);
  assert(rejected == 1 && completed.size() == 3); // Never execute overflow suffix.
  feed(std::string("arm\0disarm\n", 11), 3);
  assert(rejected == 2 && completed.size() == 3); // Embedded NUL cannot terminate a command.
  feed(std::string(1, char(0xFF)) + "arm\n", 4);
  assert(rejected == 3 && completed.size() == 3);
  feed("ar", 10); feed("m\n", 1010);
  assert(completed.back() == "arm" && rejected == 3); // Exact timeout boundary allowed.
  feed("ar", 2000);
  assert(buffer.expire(3001));
  feed("m\n", 3001);
  assert(completed.size() == 4); // Timed-out partial command is discarded, not resumed.
  feed("arm\n", 3002); assert(completed.size() == 5);
  feed("ar", UINT32_MAX - 5); feed("m\n", 4);
  assert(completed.size() == 6); // Timer rollover.
  feed("ar", 5000); feed("m\n", 6001);
  assert(rejected == 4 && completed.size() == 6); // Push also enforces expiry.
  std::puts("PASS: strict serial grammar, controller effects, fail-closed tracking, framing and rollover");
}
