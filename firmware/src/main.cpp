#include <Arduino.h>

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

void printHelp() {
  Serial.println();
  Serial.println("Tracker Bot firmware prototype");
  Serial.println("Commands:");
  Serial.println("  target <x> <y>  x/y from -1.0 to +1.0");
  Serial.println("  center          center both axes");
  Serial.println("  stop            stop tracking motion");
  Serial.println("  status          print current state");
  Serial.println("  help            show this message");
  Serial.println("Example: target 0.35 -0.20");
}

void printStatus() {
  Serial.printf("yaw=%.1f deg, pitch=%.1f deg, target=%s\n", yawAxis.angle(),
                pitchAxis.angle(), tracker.hasTarget(millis()) ? "yes" : "no");
}

void handleCommand(String command) {
  command.trim();

  if (command.equalsIgnoreCase("center")) {
    tracker.center();
    Serial.println("Centered");
    return;
  }

  if (command.equalsIgnoreCase("stop")) {
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
  if (sscanf(command.c_str(), "target %f %f", &x, &y) == 2) {
    tracker.updateTarget(x, y, millis());
    Serial.printf("Target updated: x=%.2f, y=%.2f\n", x, y);
    return;
  }

  Serial.println("Unknown command. Type 'help'.");
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    const char character = static_cast<char>(Serial.read());
    if (character == '\n' || character == '\r') {
      if (!serialLine.isEmpty()) {
        handleCommand(serialLine);
        serialLine = "";
      }
    } else if (serialLine.length() < 96) {
      serialLine += character;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  tracker.begin();
  printHelp();
}

void loop() {
  readSerialCommands();
  tracker.tick(millis());

  // Next milestone: obtain normalized x/y errors from the chosen camera and
  // lightweight detector, then call tracker.updateTarget(x, y, millis()).
}

