#include <cassert>
#include <cmath>
#include <cstring>
#include <deque>
#include "camera_target.h"
#include "camera_stream.h"

std::deque<CameraTarget> frames;
bool startCameraTarget() { return true; }
bool readCameraTarget(CameraTarget& target) {
  if (frames.empty()) return false;
  target=frames.front(); frames.pop_front(); return true;
}
// Exercise actual command/camera/control ordering, not a parallel mock app.
#include "../../firmware/src/main.cpp"

CameraTarget marker(uint32_t when) {
  return {true, 1, 0, when, 25, CameraFrameState::Marker, 1, 25};
}
void run(uint32_t now, const char* command="") {
  fakeNow=now; Serial.feed(command); loop();
}
void deliver(uint32_t now, CameraTarget target, const char* command="") {
  frames.push_back(target); run(now,command);
}
int main() {
  CameraStreamMonitor monitor(750);
  assert(!monitor.usable(0) && std::strcmp(monitor.state(0),"waiting")==0);
  monitor.observe(marker(100),100);
  assert(monitor.usable(850) && !monitor.usable(851));
  assert(std::strcmp(monitor.state(851),"timed-out")==0);
  monitor.observe(marker(100),850); // Receipt does not extend the capture lifetime.
  assert(!monitor.usable(851));
  monitor.observe(marker(900),899); // Future timestamp is ineligible.
  assert(!monitor.usable(900));
  monitor.observe(marker(UINT32_MAX-10),9);
  assert(monitor.usable(739) && !monitor.usable(740));
  for (auto state : {CameraFrameState::NoFrame, CameraFrameState::InvalidFrame,
                     CameraFrameState::StaleFrame, static_cast<CameraFrameState>(99)}) {
    auto bad=marker(100); bad.state=state; monitor.observe(bad,100);
    assert(!monitor.usable(100));
  }

  setup();
  gpio[config::kStopSensePin]=LOW;
  run(500,"arm\ncamera\n");
  assert(tracker.isArmed() && !cameraMode); // Boot-ready alone does not enable camera input.
  deliver(520,marker(520),"camera\n");
  assert(cameraMode && tracker.hasTarget(520));
  const float moved=yawAxis.angle(); assert(moved>90);
  run(1270); assert(cameraMode); // Exact 750 ms eligibility remains.
  const float held=yawAxis.angle();
  Serial.output.clear(); run(1271);
  assert(!cameraMode && !tracker.hasTarget(1271) && tracker.isArmed());
  assert(gpio[config::kPwmEnablePin]==HIGH && duty[config::kYawPwmChannel]>0);
  assert(yawAxis.angle()==held);
  assert(Serial.output.find("Camera stream fault")!=std::string::npos);
  Serial.output.clear(); run(1300);
  assert(Serial.output.empty()); // One mode-exit message, no repeated loop spam.
  deliver(1320,marker(1320));
  assert(!cameraMode && !tracker.hasTarget(1320) && yawAxis.angle()==held);
  run(1340,"camera\n");
  assert(cameraMode && !tracker.hasTarget(1340)); // Requires a subsequently delivered target.
  deliver(1360,marker(1360)); assert(tracker.hasTarget(1360));

  auto noMarker=marker(1380); noMarker.found=false; noMarker.state=CameraFrameState::NoMarker;
  noMarker.candidates=0; noMarker.selectedPixels=0;
  deliver(1380,noMarker);
  assert(cameraMode && !tracker.hasTarget(1380));
  deliver(1400,marker(1400)); assert(cameraMode && tracker.hasTarget(1400));
  auto ambiguous=noMarker; ambiguous.timestamp=1420; ambiguous.state=CameraFrameState::Ambiguous;
  ambiguous.candidates=2; deliver(1420,ambiguous);
  assert(cameraMode && !tracker.hasTarget(1420));

  uint32_t now=1440;
  for (auto state : {CameraFrameState::NoFrame, CameraFrameState::InvalidFrame,
                     CameraFrameState::StaleFrame}) {
    deliver(now,marker(now),"camera\n"); assert(cameraMode);
    auto bad=marker(now+20); bad.found=false; bad.state=state;
    deliver(now+20,bad);
    assert(!cameraMode && tracker.isArmed() && !tracker.hasTarget(now+20));
    run(now+21,"camera\n"); assert(!cameraMode);
    deliver(now+40,marker(now+40)); assert(!cameraMode);
    now+=60;
  }
  // Serial mode is independent of camera faults; a deliberate serial target still works.
  deliver(now,marker(now),"serial\ntarget 1 0\n");
  auto invalid=marker(now+20); invalid.state=CameraFrameState::InvalidFrame;
  deliver(now+20,invalid);
  assert(!cameraMode && tracker.hasTarget(now+20));
  run(now+40,"status\n");
  assert(Serial.output.find("camera_stream=invalid-frame")!=std::string::npos);

  deliver(now+60,marker(now+60),"camera\n");
  gpio[config::kStopSensePin]=HIGH;
  deliver(now+80,marker(now+80));
  assert(!cameraMode && !tracker.isArmed() && gpio[config::kPwmEnablePin]==LOW);
  gpio[config::kStopSensePin]=LOW;
  deliver(now+100,marker(now+100)); assert(!tracker.isArmed() && !cameraMode);
  run(now+120,"camera\n"); assert(cameraMode && !tracker.isArmed());
  run(now+140,"disarm\n"); assert(!cameraMode);

  run(UINT32_MAX-30,"arm\n");
  deliver(UINT32_MAX-10,marker(UINT32_MAX-10),"camera\n");
  run(739); assert(cameraMode);
  run(740); assert(!cameraMode && tracker.isArmed());
  std::puts("PASS: actual app loop camera freshness, explicit fault recovery, PWM hold, interlock, rollover");
}
