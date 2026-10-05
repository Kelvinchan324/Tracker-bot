#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
#include "app_config.h"
#include "tracker_controller.h"
#include "red_target.h"
#include "camera_frame.h"

bool near(float a, float b) { return std::fabs(a - b) < 0.001F; }

int main() {
  ServoAxis yaw(1, 0, 60, 90, 120), pitch(2, 1, 70, 90, 110);
  TrackerController tracker(yaw, pitch);
  tracker.begin();
  assert(!tracker.isArmed() && duty[0] == 0 && duty[1] == 0);
  tracker.updateTarget(1, 1, 0);
  tracker.tick(20);
  assert(near(yaw.angle(), 90) && !tracker.hasTarget(20));
  tracker.arm();
  assert(!tracker.isArmed() && gpio[config::kPwmEnablePin] == LOW);
  tracker.setInterlockClosed(true);
  tracker.arm();
  assert(gpio[config::kPwmEnablePin] == HIGH);
  assert(duty[0] > 0 && duty[1] > 0);
  tracker.updateTarget(1, -1, 0);
  tracker.tick(20);
  assert(near(yaw.angle(), 91.1F) && near(pitch.angle(), 89.1F));
  tracker.tick(700); // Catch-up motion is capped at 40 ms, not 680 ms.
  assert(near(yaw.angle(), 93.3F));
  tracker.tick(751);
  assert(!tracker.hasTarget(751) && near(yaw.angle(), 93.3F));
  fakeNow = 800;
  for (float bad : {2.0F, std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::quiet_NaN()}) {
    tracker.updateTarget(bad, 0, 800);
    assert(!tracker.hasTarget(800));
  }
  yaw.setAngle(1000); pitch.setAngle(-1000);
  assert(near(yaw.angle(), 120) && near(pitch.angle(), 70));
  yaw.setAngle(std::numeric_limits<float>::quiet_NaN());
  assert(near(yaw.angle(), 120));
  tracker.center();
  tracker.updateTarget(0.01F, -0.01F, 800);
  tracker.tick(800);
  assert(near(yaw.angle(), 90) && near(pitch.angle(), 90));
  tracker.disarm();
  assert(gpio[config::kPwmEnablePin] == LOW);
  yaw.moveBy(10); tracker.center();
  assert(!tracker.isArmed() && duty[0] == 0 && duty[1] == 0);
  assert(near(yaw.angle(), 90));

  fakeNow = UINT32_MAX - 10; tracker.arm();
  tracker.updateTarget(1, 0, fakeNow);
  tracker.tick(9); // millis rollover: 20 ms elapsed.
  assert(near(yaw.angle(), 91.1F) && tracker.hasTarget(9));
  tracker.tick(800);
  assert(!tracker.hasTarget(800));
  fakeNow = 800;
  tracker.updateTarget(1, 0, 801); // Future observations cannot become active later.
  assert(!tracker.hasTarget(801));
  tracker.updateTarget(1, 0, 49); // 751 ms old at receipt.
  assert(!tracker.hasTarget(800));
  tracker.updateTarget(1, 0, 50); // Exact 750 ms boundary is allowed.
  assert(tracker.hasTarget(800));
  tracker.setInterlockClosed(false);
  assert(!tracker.isArmed() && gpio[config::kPwmEnablePin] == LOW && duty[0] == 0);
  tracker.setInterlockClosed(true);
  assert(!tracker.isArmed()); // Release alone must not re-arm.
  tracker.arm();
  assert(tracker.isArmed());
  tracker.disarm();

  std::vector<uint8_t> frame(20 * 20 * 2, 0);
  // 25 sampled red pixels occupy the bottom-right quadrant.
  for (int y = 10; y < 20; ++y) for (int x = 10; x < 20; ++x)
    frame[(y * 20 + x) * 2] = 0xF8;
  auto target = findRedTarget(frame.data(), frame.size(), 20, 20);
  assert(target.found && target.pixels == 25 && target.x > 0 && target.y > 0);
  for (size_t i = 0; i < frame.size(); i += 2) { frame[i] = 0; frame[i+1] = 0x1F; }
  assert(!findRedTarget(frame.data(), frame.size(), 20, 20).found);
  assert(!findRedTarget(nullptr, 800, 20, 20).found);
  assert(!findRedTarget(frame.data(), 799, 20, 20).found);
  assert(!findRedTarget(frame.data(), 800, 0, 20).found);
  assert(!findRedTarget(frame.data(), 800, UINT16_MAX, 20).found);

  std::vector<uint8_t> cameraFrame(160 * 120 * 2, 0);
  for (int y = 20; y < 30; y += 2) for (int x = 100; x < 110; x += 2)
    cameraFrame[(y * 160 + x) * 2] = 0xF8;
  auto process = [&](int64_t sec, int64_t usec, int64_t now) {
    return processCameraFrame(cameraFrame.data(), cameraFrame.size(), 160, 120,
                              true, sec, usec, now, 750);
  };
  auto captured = process(2, 500000, 2600000);
  assert(captured.found && captured.pixels == 25 && captured.timestamp == 2500);
  assert(near(captured.x, 2.0F * 104 / 159 - 1) && near(captured.y, 2.0F * 24 / 119 - 1));
  assert(process(2, 0, 2750000).found);
  assert(process(2, 0, 2750001).state == CameraFrameState::StaleFrame);
  for (auto bad : {process(-1, 0, 2600000), process(2, -1, 2600000),
                   process(2, 1000000, 2600000), process(3, 0, 2600000),
                   process(2, 600001, 2600000), process(INT64_MAX, 0, INT64_MAX),
                   process(0, 0, -1)})
    assert(!bad.found && bad.state == CameraFrameState::InvalidFrame);
  assert(!processCameraFrame(cameraFrame.data(), cameraFrame.size(), 160, 120,
                             false, 2, 0, 2000000, 750).found);
  assert(!processCameraFrame(cameraFrame.data(), cameraFrame.size() - 1, 160, 120,
                             true, 2, 0, 2000000, 750).found);
  assert(!processCameraFrame(cameraFrame.data(), cameraFrame.size(), 120, 160,
                             true, 2, 0, 2000000, 750).found);
  assert(!processCameraFrame(nullptr, cameraFrame.size(), 160, 120,
                             true, 2, 0, 2000000, 750).found);
  std::fill(cameraFrame.begin(), cameraFrame.end(), 0);
  assert(process(2, 0, 2000000).state == CameraFrameState::NoMarker);
  for (int x = 0; x < 19; ++x) cameraFrame[x * 4] = 0xF8;
  assert(!process(2, 0, 2000000).found && process(2, 0, 2000000).pixels == 19);
  cameraFrame[19 * 4] = 0xF8;
  assert(process(2, 0, 2000000).found && process(2, 0, 2000000).pixels == 20);
  for (size_t i = 0; i < cameraFrame.size(); i += 2) {
    cameraFrame[i] = 0x07; cameraFrame[i + 1] = 0xE0; // Pure green.
  }
  assert(!process(2, 0, 2000000).found);
  std::fill(cameraFrame.begin(), cameraFrame.end(), 0xFF); // White is not red.
  assert(!process(2, 0, 2000000).found);
  for (size_t i = 0; i < cameraFrame.size(); i += 2) {
    cameraFrame[i] = 0xF8; cameraFrame[i + 1] = 0;
  }
  const uint64_t rolloverUs = (uint64_t(UINT32_MAX) + 11) * 1000;
  auto rolled = process(rolloverUs / 1000000, rolloverUs % 1000000, rolloverUs + 1000);
  assert(rolled.found && rolled.timestamp == 10);
  fakeNow = 2800;
  tracker.setInterlockClosed(true); tracker.arm();
  tracker.updateTarget(captured.x, captured.y, captured.timestamp);
  assert(tracker.hasTarget(3250) && !tracker.hasTarget(3251)); // No refreshed timestamp.
  tracker.stopTracking(); // A delivered invalid/no-marker frame immediately stops increments.
  assert(!tracker.hasTarget(fakeNow) && tracker.isArmed());
  tracker.updateTarget(captured.x, captured.y, captured.timestamp);
  assert(tracker.hasTarget(fakeNow)); // Reacquisition remains automatic in camera mode.
  tracker.disarm();
  std::puts("PASS: arming, PWM disable, limits, timeout, dt cap, deadband, rollover, finite inputs, RGB565 centroid");
  std::puts("PASS: camera metadata, capture age, future/stale rejection, timestamp rollover");
}
