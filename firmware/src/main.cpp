#include <Arduino.h>
#include <math.h>
#include "camera_target.h"

#include "app_config.h"
#include "servo_axis.h"
#include "tracker_controller.h"

ServoAxis yawAxis(config::kYawServoPin, config::kYawPwmChannel,
                  config::kYawMinDeg, config::kYawCenterDeg,
                  config::kYawMaxDeg);
ServoAxis pitchAxis(config::kPitchServoPin, config::kPitchPwmChannel,
                    config::kPitchMinDeg, config::kPitchCenterDeg,
                    config::kPitchMaxDeg);
TrackerController tracker(yawAxis, pitchAxis);

String serialLine;
bool serialOverflow = false;
bool cameraReady = false;
bool cameraMode = false;

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
}

void printStatus() {
  Serial.printf("armed=%s, input=%s, camera=%s, yaw=%.1f deg, pitch=%.1f deg, target=%s\n",
                tracker.isArmed() ? "yes" : "no", cameraMode ? "camera" : "serial",
                cameraReady ? "ready" : "unavailable", yawAxis.angle(),
                pitchAxis.angle(), tracker.hasTarget(millis()) ? "yes" : "no");
  Serial.printf("stop_contact=%s\n", tracker.interlockClosed() ? "closed" : "OPEN: arm inhibited");
}

void handleCommand(String command) {
  command.trim();

  if (command == "arm") {
    tracker.setInterlockClosed(digitalRead(config::kStopSensePin) == LOW);
    tracker.arm();
    Serial.println(tracker.isArmed() ? "Armed: last commanded position applied" : "Blocked: stop contact open");
    return;
  }
  if (command == "disarm") { cameraMode = false; tracker.disarm(); return; }
  if (command == "camera") {
    tracker.stopTracking();
    cameraMode = cameraReady;
    Serial.println(cameraMode ? "Red marker camera mode" : "Camera unavailable");
    return;
  }
  if (command == "serial") { cameraMode = false; tracker.stopTracking(); return; }

  if (command.equalsIgnoreCase("center")) {
    cameraMode = false;
    tracker.center();
    Serial.println(tracker.isArmed() ? "Center commanded; serial mode" : "Ignored: disarmed");
    return;
  }

  if (command.equalsIgnoreCase("stop")) {
    cameraMode = false;
    tracker.stopTracking();
    Serial.println("Tracking stopped");
    return;
  }

  if (command.equalsIgnoreCase("status")) {
    printStatus();
    return;
  }

  if (command.equalsIgnoreCase("help")) {
    printHelp();
    return;
  }

  float x = 0.0F;
  float y = 0.0F;
  char extra = 0;
  if (sscanf(command.c_str(), "target %f %f %c", &x, &y, &extra) == 2 &&
      isfinite(x) && isfinite(y) && fabsf(x) <= 1 && fabsf(y) <= 1 &&
      tracker.isArmed() && !cameraMode) {
    tracker.updateTarget(x, y, millis());
    Serial.printf("Target updated: x=%.2f, y=%.2f\n", x, y);
    return;
  }

  Serial.println("Unknown command. Type 'help'.");
}

void readSerialCommands() {
  // Bound serial work so a continuous input stream cannot starve the stop check.
  uint16_t consumed = 0;
  while (Serial.available() > 0 && consumed++ < 128) {
    const char character = static_cast<char>(Serial.read());
    if (character == '\n' || character == '\r') {
      if (!serialOverflow && !serialLine.isEmpty()) {
        handleCommand(serialLine);
      }
      serialLine = "";
      serialOverflow = false;
    } else if (!serialOverflow && serialLine.length() < 96) {
      serialLine += character;
    } else {
      serialOverflow = true;
      serialLine = "";
      cameraMode = false;
      tracker.stopTracking();
    }
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
  readSerialCommands();
  CameraTarget target{};
  if (readCameraTarget(target) && cameraMode) {
    if (target.found && millis() - target.timestamp <= config::kTargetTimeoutMs)
      tracker.updateTarget(target.x, target.y, target.timestamp);
    else tracker.stopTracking();
  }
  tracker.tick(millis());

  delay(1);
}

