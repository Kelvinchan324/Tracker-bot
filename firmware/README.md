# Tracker Bot firmware

This directory is a basic PlatformIO project for an ESP32-S3. It provides a
safe starting point for testing yaw and pitch with two PWM hobby servos before
the final camera, AI model, and actuator hardware are selected.

## What works now

- two-axis PWM servo output;
- configurable pins, travel limits, center positions, and control gains;
- normalized target-error input (`-1.0` to `+1.0`);
- deadband to reduce servo hunting;
- automatic stop when target updates time out;
- USB serial commands for testing without a camera.

This version does **not** yet contain camera capture or AI inference. It also
does not directly drive a brushless gimbal motor. A gimbal motor requires its
specific driver, feedback sensor, and control protocol.

## Open in VS Code

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the **PlatformIO IDE** extension.
3. In PlatformIO, choose **Open Project** and select this `firmware` folder.
4. Wait for PlatformIO to install the ESP32 toolchain.
5. Build the `esp32-s3-devkitc-1` environment.

If your board is not an ESP32-S3 DevKitC-1, change `board` in
`platformio.ini` to the exact PlatformIO board identifier.

## Wiring for the first bench test

| Signal | Default pin |
| --- | --- |
| Yaw servo PWM | GPIO 4 |
| Pitch servo PWM | GPIO 5 |

Power the motors from a suitable external supply. Connect the motor-supply
ground to ESP32 ground. Do not power motors from the ESP32 3.3 V pin, and do
not attach the camera mechanism until direction and travel limits have been
verified with the servos unloaded.

Pins and motion settings are in `include/app_config.h`. The defaults are only
placeholders because the final ESP32 camera board and motor hardware have not
been chosen.

## Test through the serial monitor

Upload the firmware, open the PlatformIO serial monitor at 115200 baud, and
send commands ending with a newline:

```text
help
status
target 0.35 -0.20
center
stop
```

For `target x y`, negative/positive `x` means left/right and negative/positive
`y` means up/down. If an installed axis moves away from the target, reverse
that axis sign in `TrackerController::tick()` before further testing.

Target commands need to arrive at least once every 750 ms. Otherwise motion
stops automatically. This simulates the safety behavior needed when the vision
system loses its target.

## Suggested next milestones

1. Confirm the exact ESP32-S3 camera board and its occupied pins.
2. Confirm whether the prototype uses PWM servos or closed-loop gimbal motors.
3. Bench-test direction, center, current draw, and mechanical travel.
4. Add camera capture and output normalized target coordinates.
5. Integrate a lightweight detector only after camera capture is stable.
6. Add a hardware emergency stop, calibration, and stall protection.

