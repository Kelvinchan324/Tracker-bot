#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
#include "app_config.h"
#include "tracker_controller.h"
#include "red_target.h"

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
  assert(duty[0] > 0 && duty[1] > 0);
  tracker.updateTarget(1, -1, 0);
  tracker.tick(20);
  assert(near(yaw.angle(), 91.1F) && near(pitch.angle(), 89.1F));
  tracker.tick(700); // Catch-up motion is capped at 40 ms, not 680 ms.
  assert(near(yaw.angle(), 93.3F));
  tracker.tick(751);
  assert(!tracker.hasTarget(751) && near(yaw.angle(), 93.3F));
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
  yaw.moveBy(10); tracker.center();
  assert(!tracker.isArmed() && duty[0] == 0 && duty[1] == 0);
  assert(near(yaw.angle(), 90));

  fakeNow = UINT32_MAX - 10; tracker.arm();
  tracker.updateTarget(1, 0, fakeNow);
  tracker.tick(9); // millis rollover: 20 ms elapsed.
  assert(near(yaw.angle(), 91.1F) && tracker.hasTarget(9));
  tracker.tick(800);
  assert(!tracker.hasTarget(800));

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
  std::puts("PASS: arming, PWM disable, limits, timeout, dt cap, deadband, rollover, finite inputs, RGB565 centroid");
}
