#include <Arduino.h>
#include <math.h>
#include "camera_target.h"
#include "camera_frame.h"
#include "camera_stream.h"

#include "app_config.h"
#include "servo_axis.h"
#include "tracker_controller.h"
#include "serial_command.h"

ServoAxis yawAxis(config::kYawServoPin, config::kYawPwmChannel,
                  config::kYawMinDeg, config::kYawCenterDeg,
                  config::kYawMaxDeg);
ServoAxis pitchAxis(config::kPitchServoPin, config::kPitchPwmChannel,
                    config::kPitchMinDeg, config::kPitchCenterDeg,
                    config::kPitchMaxDeg);
TrackerController tracker(yawAxis, pitchAxis);

SerialLineBuffer serialInput;
bool cameraReady = false;
bool cameraMode = false;
CameraStreamMonitor cameraStream(config::kTargetTimeoutMs);
CameraTarget lastCameraTarget{false, 0, 0, 0, 0, CameraFrameState::NoFrame, 0, 0};

void printHelp() {
  Serial.println();
  Serial.println("Tracker Bot firmware prototype");
  Serial.println("Commands:");
  Serial.println("  arm / disarm    explicit PWM enable / disable (support payload)");
  Serial.println("  camera / serial choose marker camera or serial target input");
  Serial.println("  target <x> <y>  x/y from -1.0 to +1.0");
  Serial.println("  center          center both axes");
  Serial.println("  stop            stop tracking motion");
  Serial.println("  status          print current state");
  Serial.println("  help            show this message");
  Serial.println("Example: target 0.35 -0.20");
  Serial.println("ASCII lines: max 96 bytes; finish within 1 s; invalid input stops tracking, not PWM");
}

void printStatus() {
  Serial.printf("armed=%s, input=%s, camera=%s, yaw=%.1f deg, pitch=%.1f deg, target=%s\n",
                tracker.isArmed() ? "yes" : "no", cameraMode ? "camera" : "serial",
                cameraReady ? "ready" : "unavailable", yawAxis.angle(),
                pitchAxis.angle(), tracker.hasTarget(millis()) ? "yes" : "no");
  Serial.printf("stop_contact=%s\n", tracker.interlockClosed() ? "closed" : "OPEN: arm inhibited");
  Serial.printf("camera_stream=%s\n", cameraReady ? cameraStream.state(millis()) : "unavailable");
  Serial.printf("last_frame=%s, red_samples=%lu, candidates=%u, selected_samples=%lu",
                cameraFrameStateName(lastCameraTarget.state),
                static_cast<unsigned long>(lastCameraTarget.pixels),
                static_cast<unsigned>(lastCameraTarget.candidates),
                static_cast<unsigned long>(lastCameraTarget.selectedPixels));
  if (lastCameraTarget.state == CameraFrameState::Marker ||
      lastCameraTarget.state == CameraFrameState::NoMarker ||
      lastCameraTarget.state == CameraFrameState::Ambiguous ||
      lastCameraTarget.state == CameraFrameState::StaleFrame)
    Serial.printf(", capture_age_ms=%lu", static_cast<unsigned long>(millis() - lastCameraTarget.timestamp));
  Serial.println();
}

void handleCommand(const char* line) {
  const auto command = parseSerialCommand(line);
  const char* message = applySerialCommand(command, tracker, cameraMode,
      cameraReady && cameraStream.usable(millis()),
      digitalRead(config::kStopSensePin) == LOW, millis());
  if (message) Serial.println(message);
  if (command.kind == CommandKind::Status) printStatus();
  if (command.kind == CommandKind::Help) printHelp();
}

void rejectSerialLine() {
  cameraMode = false; tracker.stopTracking();
  Serial.println("Invalid or expired line: tracking stopped; send newline then a new command");
}

void readSerialCommands() {
  // Bound serial work so a continuous input stream cannot starve the stop check.
  if (serialInput.expire(millis())) rejectSerialLine();
  uint16_t consumed = 0;
  while (Serial.available() > 0 && consumed++ < 128) {
    const auto event = serialInput.push(static_cast<uint8_t>(Serial.read()), millis());
    if (event == LineEvent::Complete) handleCommand(serialInput.line());
    if (event == LineEvent::Rejected) rejectSerialLine();
  }
}

void setup() {
  tracker.begin();
  pinMode(config::kStopSensePin, INPUT_PULLUP);
  Serial.begin(115200);
  delay(500);
  cameraReady = startCameraTarget();
  Serial.println(cameraReady ? "Camera ready; motors DISARMED" : "Serial-only; motors DISARMED");
  printHelp();
}

void loop() {
  const bool stopClosed = digitalRead(config::kStopSensePin) == LOW;
  tracker.setInterlockClosed(stopClosed);
  if (!stopClosed) cameraMode = false;
  CameraTarget target{};
  const bool received = readCameraTarget(target);
  if (received) {
    lastCameraTarget = target;
    cameraStream.observe(target, millis());
  }
  // Poll health before commands: camera selection needs a currently fresh image,
  // not merely successful initialization at boot.
  if (cameraMode && !cameraStream.usable(millis())) {
    cameraMode = false;
    tracker.stopTracking();
    Serial.println("Camera stream fault: tracking stopped; PWM held. Fresh frames + camera required");
  }
  readSerialCommands();
  if (received && cameraMode) {
    if (target.found && millis() - target.timestamp <= config::kTargetTimeoutMs)
      tracker.updateTarget(target.x, target.y, target.timestamp);
    else tracker.stopTracking();
  }
  tracker.tick(millis());

  delay(1);
}

