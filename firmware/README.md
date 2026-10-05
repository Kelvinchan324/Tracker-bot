# Tracker Bot firmware — EVT-A

Experimental two-axis servo firmware with a red-marker camera path.
It has compiled for both boards below; physical operation is not validated.
Read the [engineering build and teaching manual](../docs/engineering.md) before
wiring or uploading. This is not brushless-gimbal firmware or person recognition.

| Environment | Board | Yaw / pitch | Camera |
| --- | --- | --- | --- |
| `xiao-sense` (default) | XIAO ESP32-S3 Sense | GPIO1 / GPIO2 | RGB565 red-marker centroid |
| `esp32-s3-devkitc-1` | ESP32-S3 DevKitC-1 | GPIO4 / GPIO5 | Serial targets only |

EVT-A.1 also requires translator OE and the separate NC power-cut auxiliary
contact: XIAO GPIO3/GPIO4 respectively, DevKit GPIO6/GPIO7 respectively.
Open auxiliary contact (or disconnected cable) inhibits arm. Wire the new
TXU0102 from the current schematic, not the obsolete AHCT125 DIP14 circuit.

## Build and upload

Install PlatformIO Core or the PlatformIO extension in VS Code. Open this folder.
Run `pio run` to compile the default board. Only after wiring review, use
`pio run -t upload` and `pio device monitor` (115200 baud).
Choose the other target explicitly with `-e esp32-s3-devkitc-1`.

## Supported bench sequence

Start with horns removed, mechanism restrained, current-limited motor supply
and a reachable physical power cut. USB powers the MCU, not the motors.
Use the buffer and separate regulated motor supply in the
[module schematic](../hardware/schematic.svg); do not connect motors to 3.3 V.

Send newline-terminated commands:

```text
status
arm
target 0.2 0
stop
center
camera
stop
disarm
```

- Boot is disarmed; `arm` applies the last commanded position (initially centre).
  A closed auxiliary contact is required. Opening it disarms; reclosing it alone
  does not restart motion. An external motor-rail loss without contact opening
  is not detected: disarm before reconnecting power.
- `target x y` accepts finite values in [-1,1], in armed serial mode only.
  Negative x/y means left/up. Actual installed directions need a hardware check.
- Targets expire after 750 ms; the last pulse is held. There is no encoder feedback.
- `camera` enables marker input if capture initialized; it does not arm motors.
- `serial`, `stop` and `center` leave camera tracking; `center` needs arming.
- `disarm` removes PWM but is not a physical power cut; support the payload.
- `status` reports arming, input mode, camera state, commanded angles and target.
- Pins, ranges, deadband and gain are in `include/app_config.h`.

Use [host logic tests](../tests/README.md) and compile both environments after
changes. Neither compilation nor mocked tests proves electrical or mechanical safety.
