# Camera input and calibration lesson

Revision: 5 October 2026. **Unexecuted physical worksheet.** Native software tests
and embedded builds are not evidence that a real camera or mechanism works.

## Frame contract and corrected age

The reference mode is tightly packed, MSB-first RGB565 at 160x120 (38,400 bytes).
The capture boundary rejects other metadata before reading pixels. Red is a
simple channel-dominance threshold, sampled on every second row/column, with a
minimum of 20 red samples. It is not an object classifier or connected-component
detector. Multiple red objects, reflections and scattered noise can contribute
to the same centroid. A one-pixel shift can change sampled counts.

The old path assigned the time of frame retrieval, allowing a buffered old
image to appear recent. The corrected path preserves capture-start time, rejects
invalid or future times and ages frames against the boot-relative timer before
processing. It retains that timestamp through the queue and controller; a target
is eligible through age 750 ms and expires beyond it. This is a chosen software
limit, not a measured safety stop time. At consumption, ordinary control-loop
scheduling adds latency; a frozen MCU is not covered by these checks.

Evidence: the installed Arduino-ESP32 2.0.17 camera header describes the timestamp
as boot-relative first-DMA-buffer time. Upstream
[camera acquisition source](https://github.com/espressif/esp32-camera/blob/v2.0.4/driver/cam_hal.c)
assigns it using `esp_timer_get_time`; Arduino 2.0.17
[`millis()` implementation](https://github.com/espressif/arduino-esp32/blob/2.0.17/cores/esp32/esp32-hal-misc.c)
uses that timer divided by 1000. The installed header SHA-256 is
`fb13779551477d5e9baf2af721e3a9d5e4625ba66114b687d25ed15e3edffd23`.
This source/header review is not a measurement of the bundled camera driver's
timing on either sensor revision; verify with the actual board.

## Operator behaviour

| Observation | Control response while armed in camera mode |
| --- | --- |
| Fresh valid marker | Bounded incremental targets; no encoder feedback |
| Delivered invalid/stale/no-marker/no-frame result | Stop new increments; keep last PWM |
| Capture task stalls or queue stays silent | Last capture-based target expires; keep last PWM |
| Fresh marker returns | Tracking automatically resumes while camera mode remains selected |
| `stop` or `serial` | Exit camera mode and stop tracking increments |
| `disarm` or open auxiliary stop contact | Disable PWM/OE; payload may fall; physical power cut still required |

`status` prints `last_frame`, `red_samples`, and (where a timestamp is valid)
`capture_age_ms`. An old `marker` label can persist after a task hangs: it means
last received result, not a continuously healthy camera. Check age and `target`.
Camera processing remains active in serial/disarmed modes; no network upload is
implemented. `stop` is not a camera privacy shutter or motor-power isolation.

## Teaching procedure

1. With motors disconnected, run native tests. Change a synthetic frame timestamp
   from 750,000 to 750,001 microseconds old and explain the boundary. Contrast
   capture, retrieval, processing and command time.
2. Identify the actual camera/sensor/PCB revision. Keep the mechanism restrained;
   leave motors disconnected for the image tests. Use one red card on a plain
   background. Record left/right/top/bottom placement, red sample counts and
   lighting. Byte order and orientation must be established, not assumed.
3. Under the separate approved low-energy bench procedure, verify each actuator
   direction independently. Initial arming applies stored position and can move
   abruptly. Default `target` input is a velocity-like error, NOT a one-degree
   command; do not infer physical angle from the serial input. Use externally
   measured displacement and the physical power cut as needed.
4. Record whether a small positive actuator change reduces or increases each
   image error. The current positive-gain control requires its own-axis error to
   decrease for a positive actuator change. Correct polarity only after measured
   evidence; record cross-axis response too. The worksheet is not camera intrinsic
   calibration, an optical-frame transform, or a closed-loop stability proof.
5. Test loss and re-entry with the supported no-payload mechanism. Record PWM,
   commanded angle and actual motion separately. Demonstrate that `stop` prevents
   camera-driven resumption and that contact release alone does not arm.
6. Keep failed trials and environmental conditions. Do not tune gains or widen
   travel to hide poor detection, incorrect polarity or mechanical interference.

## Physical worksheet — all results pending

| Record | Actual result / evidence file |
| --- | --- |
| Operator, observer, date, firmware commit/build environment | pending |
| Board/sensor marking, lens, PCB revision and photos | pending |
| Independent restraint, power cut, current limit and horns/payload configuration | pending |
| Captured format/dimensions, byte-order evidence, sensor mirror/flip | pending |
| Marker physical size, distance, lighting/background for each trial | pending |
| Four image quadrants: observed direction and red sample counts | pending |
| Measured yaw change versus horizontal/vertical image-error change | pending |
| Measured pitch change versus horizontal/vertical image-error change | pending |
| Capture-age distribution and independent timing method | pending |
| Marker loss: last capture, last incremental PWM command, physical stopping | pending |
| Re-entry behaviour, two-red-object and non-red distractor trials | pending |
| Stop/serial/disarm behaviour and separate power-cut response | pending |
| Accepted polarity, gain, bounds, unresolved issues and reviewer signoff | pending |

Do not copy simulation outputs into measured fields. Production needs actual
identity/target selection, obstruction/current sensing, validated mechanics and
independent fault supervision; this educational marker prototype is not finished.
