#include <cassert>
#include <cmath>
#include <cstdio>
#include "camera_target.h"
bool startCameraTarget() { return false; }
bool readCameraTarget(CameraTarget&) { return false; }
#include "../../firmware/src/main.cpp"

void expectDisabled() {
  assert(!tracker.isArmed());
  assert(gpio[config::kPwmEnablePin]==LOW);
  for (const auto& event : outputEvents)
    assert(!(event.kind=='G' && event.index==config::kPwmEnablePin && event.value==HIGH));
}

int main() {
  assert(config::kServoPwmResolutionBits==14);
  // Real setup() must expose the initialization failure without enabling outputs.
  setupFails[config::kYawPwmChannel]=true;
  setup();
  assert(!tracker.pwmReady()); expectDisabled();
  assert(Serial.output.find("PWM initialization FAILED")!=std::string::npos);
  gpio[config::kStopSensePin]=LOW;
  handleCommand("arm"); expectDisabled();
  handleCommand("status");
  assert(Serial.output.find("pwm_setup=FAILED")!=std::string::npos);
  assert(Serial.output.find("Blocked: PWM initialization failed")!=std::string::npos);

  // Either timer-setup result can inhibit both axes, including a wrong nonzero frequency.
  for (const auto failed : {config::kYawPwmChannel,config::kPitchPwmChannel}) {
    for (const uint32_t returned : {0U,49U,51U}) {
      std::fill(std::begin(setupFails),std::end(setupFails),false);
      std::fill(std::begin(setupResult),std::end(setupResult),0);
      setupFails[failed]=returned==0; setupResult[failed]=returned;
      outputEvents.clear(); tracker.begin();
      assert(!tracker.pwmReady());
      tracker.setInterlockClosed(true); tracker.arm(); expectDisabled();
      tracker.updateTarget(1,1,millis()); tracker.tick(millis()+20);
      tracker.center(); expectDisabled();
      for (const auto& event : outputEvents) {
        if (event.kind=='A') assert(event.index!=failed); // Never attach failed axis.
        if (event.kind=='W') assert(event.value==0);
      }
      setupFails[failed]=false; setupResult[failed]=0;
      handleCommand("arm"); expectDisabled(); // No blind automatic retry.
    }
  }

  outputEvents.clear(); tracker.begin();
  assert(tracker.pwmReady()); expectDisabled();
  assert(outputEvents.front().kind=='G' && outputEvents.front().value==LOW);
  bool zeroed[2]={false,false};
  for (const auto& event : outputEvents) {
    if (event.kind=='W' && event.value==0) zeroed[event.index]=true;
    if (event.kind=='A') assert(zeroed[event.index]); // Attach inherits zero duty.
  }
  assert(setupFrequency[0]==50 && setupFrequency[1]==50 && setupBits[0]==14 && setupBits[1]==14);
  tracker.arm(); expectDisabled(); // begin() also resets contact observation.
  tracker.setInterlockClosed(true);
  outputEvents.clear(); tracker.arm();
  assert(tracker.isArmed());
  bool pulseSet[2]={false,false};
  for (const auto& event : outputEvents) {
    if (event.kind=='W' && event.value>0) pulseSet[event.index]=true;
    if (event.kind=='G' && event.value==HIGH) assert(pulseSet[0] && pulseSet[1]);
  }
  // Independent pulse calculation: actual period has 2^14 counts, not 2^14-1.
  for (float angle=60; angle<=120; angle+=.125F) {
    yawAxis.setAngle(angle);
    const double expected=1000.0+1000.0*angle/180.0;
    const double actual=duty[config::kYawPwmChannel]*20000.0/16384.0;
    assert(std::fabs(actual-expected)<=20000.0/16384.0/2+.0001);
  }
  yawAxis.center();
  assert(duty[config::kYawPwmChannel]==1229); // 1500.244 us nominal, NOT measured.
  outputEvents.clear(); tracker.setInterlockClosed(false);
  assert(outputEvents.front().kind=='G' && outputEvents.front().value==LOW);
  expectDisabled(); assert(duty[0]==0 && duty[1]==0);
  tracker.setInterlockClosed(true); assert(!tracker.isArmed());
  tracker.arm(); assert(tracker.isArmed());
  outputEvents.clear(); tracker.begin(); // Explicit reinitialization must first disarm.
  expectDisabled(); assert(duty[0]==0 && duty[1]==0);
  assert(!tracker.interlockClosed());
  handleCommand("status");
  assert(Serial.output.find("pwm_setup=ready")!=std::string::npos);

  // A directly used axis also refuses enable before setup or after setup failure.
  ServoAxis direct(9,2,60,90,120);
  outputEvents.clear(); direct.enable(); direct.setAngle(110);
  assert(outputEvents.empty() && direct.angle()==90);
  setupFails[2]=true; direct.begin(); direct.enable();
  assert(!direct.isReady() && direct.angle()==90);
  std::puts("PASS: S3 14-bit PWM, setup failures, arm inhibition, OE ordering, pulse quantization, restart");
}
