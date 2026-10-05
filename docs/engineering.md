# Tracker Bot EVT-A engineering draft

Revision A.1 chooses a small, wired desk demonstrator: XIAO ESP32-S3 Sense,
two SG90 hobby servos, a separate regulated 4.8 V motor supply and a bright red
marker. It implements an experimental camera-to-motion path; person recognition, quiet
gimbal actuation, battery operation and a consumer enclosure remain future work.

## Controlled files

- [Electrical schematic](../hardware/schematic.svg): labelled module pins and nets.
- [BOM and net list](../hardware/BOM.md): quantities, source and unresolved selection.
- [Revision source](../hardware/rev-a.json): editable electrical/mechanical assumptions.
- [Placement drawing](../mechanical/placement.svg) and [STEP layout](../mechanical/layout-draft.step).
- [Fixture STEP](../mechanical/fixture-plate.step) and [STL](../mechanical/fixture-plate.stl).
- [CAD/document generator](../tools/build_engineering.py).

The STEP assembly contains labelled component bounding boxes and a drilled fixture
plate. It is a packaging study, not accurate servo internals, a finished pan/tilt
mechanism or print-approved parts. Hole locations on the plate are for fixture
attachment only. Horn centres, spline, screws and moving brackets are not released.
Two [parametric mounting candidates](../mechanical/README.md) add a yaw-body
seat and camera saddle. They do not yet connect through a rotating carrier.

## Electrical rationale

The camera board receives USB power. A separate 4.8 V supply feeds F1, then the
latching NC power-cut contact S1, then both servos and the buffer. Grounds join
at the motor connector. Never route servo supply current through XIAO pins,
USB ground jumpers or solderless breadboard contacts. Use short adequately rated
power leads, keyed connectors, strain relief and insulated terminations.

U2 is now a TXU0102 dual-supply translator, not the previous AHCT125.
VCCA receives MCU 3.3 V and VCCB receives switched motor power; grounds are common.
The [power and interlock review](power-and-interlock.md) gives the IC pin mapping,
pull resistors, two-rail bypass capacitors, wiring schedule and power-state tests.
Do not reuse the previous DIP14 wiring. The new part is VSSOP8 and needs a
verified breakout. This is signal isolation on power-off, not galvanic isolation.
Hardware back-feed, timing and fault tests remain mandatory.

Source facts: [Seeed pin map](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/),
[Seeed camera guide](https://wiki.seeedstudio.com/xiao_esp32s3_camera_usage/),
[TowerPro SG90](https://towerpro.com.tw/product/sg90-analog/),
[TI TXU0102](https://www.ti.com/lit/ds/symlink/txu0102.pdf). Accessed 5 October 2026.
Supplier listings and measured samples must settle signal thresholds, current
peaks and actuator travel. F1=2 A is a starting engineering assumption, not a
validated protection coordination result.

## Mechanics and placement

Coordinates are millimetres: origin at plate XY centre and bench height Z=0;
+X is forward, +Y left, +Z up. M1 is the yaw motor, shaft along +Z.
M2 is the pitch motor, shaft along +Y. The camera lens should face +X;
its electronics envelope is centred at (15,0,87). Keep its antenna free of
metal and keep wires outside the two swept volumes.

The 100 x 80 x 4 mm base has four 3.4 mm fixture holes at (+/-43,+/-33).
A 23 x 12.2 x 29 mm servo body envelope comes from the manufacturer;
horn and mounting-ear dimensions need measurement. Put a bearing opposite the
pitch servo if payload or bracket flex requires it. Do not load the servo shaft
as the sole cantilever support without a load check.

Sizing example, not a tested payload: a 50 g camera assembly with centre of
mass 20 mm from pitch axis gives 0.05 x 9.81 x 0.02 = 0.00981 Nm static torque.
Cable drag, acceleration, backlash and continuous duty add load. Published stall
torque is not a continuous operating rating. Record actual payload mass, offset,
steady current, stall response and temperature before increasing it.

## Software behaviour

Default environment `xiao-sense` pins the ESP32 platform to 6.10.0.
Camera runs in a separate task and communicates via a one-entry queue; the
control loop uses the newest observation. Camera XCLK uses LEDC timer 3/channel 7;
servo PWM uses channels 0/1. No network service or cloud processing is started.

Boot produces no PWM until `arm`. Arming applies the current stored position
(initially 90 degrees), so support the mechanism and fit horns at centre first.
`stop` stops tracking and holds the last PWM; `disarm` removes pulses, which
may let a payload fall. S1 physically interrupts motor power.
Its separate NC auxiliary contact to ground is monitored on GPIO4 (XIAO) or
GPIO7 (DevKit). An open contact/wire disarms and disables translator OE. Releasing
S1 does not re-arm; a new `arm` command is required. This loop-polled input is not
a certified or hardware-latched safety interlock.
Neither is a certified emergency-stop function.

Targets outside [-1,1], nonfinite values, trailing text and overflowing lines are
rejected. Lost or stale targets stop incremental motion after 750 ms.
Control step duration is capped at 40 ms to prevent catch-up jumps.
Default nominal software ranges are yaw 60–120, pitch 70–110 degrees, with
1000–2000 microsecond mapping. They are command coordinates, not encoder readings.

Marker mode thresholds red pixels in 160 x 120 RGB565 frames. It is sensitive to
lighting and background objects. Two red objects yield a combined centroid.
Verify byte order, sensor orientation and motor polarity on the actual camera;
OV2640 and OV3660 board revisions require separate checks.

## Teaching sequence and acceptance evidence

1. **Electronics (45 min):** with motors disconnected, identify all schematic
   nets using continuity checks. Measure regulated rails and PWM with a scope.
   Evidence: annotated schematic, measured rail values, centre pulse and power-cut result.
2. **Control (45 min):** horns removed; upload, confirm no startup pulse, arm,
   send `target 0.2 0`, then stop. Evidence: bounded direction, timeout,
   NaN/range rejection and power-cycle transcript. Reverse one axis in code if needed.
3. **Vision (60 min):** put a red card in a plain background; run `camera`.
   Evidence: marker centring, loss, distractor and re-entry clips under two lighting conditions.
4. **Mechanical review (45 min):** measure the actual parts and revise envelopes.
   Evidence: calliper table, cable sweep, finger clearance and stable-base test.

The product draft is not finished until brackets/horn connections, current limits,
sensor orientation, endurance and the user's desired target class have evidence.

The [candidate carrier/frame lesson](../mechanical/carrier-review.md) now adds
editable connecting solids, revised plate holes and 25 sampled pose checks. It
does not replace the physical mechanical review or increase the commanded travel.
