# PWM startup correction and no-hardware lesson

Status: software/source-verified correction; **no measured servo waveform**.

## Why this was necessary

Both board configurations target ESP32-S3. Its pinned SDK capability header
declares eight LEDC channels and a maximum 14-bit timer width. The older Tracker
setting requested 16 bits. Arduino's setup implementation returns zero for an
unsupported width, but Tracker ignored that result. Compilation and the earlier
host shim therefore passed without establishing that PWM could initialize.

Primary sources reviewed:

- [ESP-IDF v4.4.7 ESP32-S3 LEDC capability header](https://github.com/espressif/esp-idf/blob/v4.4.7/components/soc/esp32s3/include/soc/ledc_caps.h).
- [Arduino-ESP32 2.0.17 LEDC implementation](https://github.com/espressif/arduino-esp32/blob/2.0.17/cores/esp32/esp32-hal-ledc.c): setup returns frequency or zero; attach inherits channel duty; attach/write do not return an application-checkable success result.

The locally pinned PlatformIO package is
`framework-arduinoespressif32 3.20017.241212+sha.dcc1105b`; its reviewed
`cores/esp32/esp32-hal-ledc.c` SHA-256 is
`4069ddc26b5ef35b35dd8e37977d20d31e7a13387839b9c7c9dc365f583b23c7`.
No dependency upgrade was made.

## Corrected behavior

1. Hold translator OE low before initializing either axis. Reset tracking,
   arming and the remembered contact observation during initialization.
2. Request 50 Hz with 14-bit resolution. Embedded compile-time checks reject a
   resolution/channel outside the chip's declared capabilities.
3. Require a returned frequency of exactly 50 Hz for each axis. On failure,
   do not attach that axis; neither axis may arm. This is a deliberately strict
   software result check, not a measurement of the clock.
4. Write zero duty before attaching each successful axis's pin. Explicit `arm`
   still requires a closed stop auxiliary contact. Program both stored pulse
   targets before raising OE. Disarm lowers OE before clearing pulse duty.
5. Report `pwm_setup=ready` or `pwm_setup=FAILED` in `status`. Failure is retained
   until deliberate reinitialization/restart; repeatedly sending `arm` does not
   retry setup or override it. Support the mechanism and isolate motor power first.

No pin/net, servo travel limit, target gain or camera timer assignment changed.
Servo channels 0/1 share timer 0; camera XCLK remains channel 7/timer 3. The gate
does not detect a later timer reconfiguration, disconnected signal wire, wrong
pin routing, failed `ledcAttachPin`/`ledcWrite` or bad physical output.

## Pulse arithmetic, not measurement

At nominal 50 Hz the period is 20,000 microseconds. A 14-bit period has 16,384
counts, or about 1.220703 microseconds per count. Duty conversion now rounds to
the nearest count using that full period count, rather than scaling by 16,383
and truncating. The 90-degree software command maps to 1,500 microseconds,
rounded to count 1,229 (nominal 1,500.244 microseconds). Quantization alone is at
most about 0.610352 microseconds; oscillator error and real hardware jitter are
not included. Commanded degrees still do not prove a measured servo angle.

## Teaching exercise (30 minutes, no motors)

1. Build and run `test_pwm_init.cpp` using [the test instructions](../tests/README.md).
2. Predict the result when yaw setup returns zero but pitch succeeds. Verify
   that a closed contact and repeated `arm` cannot raise OE.
3. Repeat for the pitch axis and unexpected nonzero frequency results. Explain
   why “nonzero” alone would not validate the pulse conversion assumptions.
4. Trace the host-recorded output order for successful startup, arm and disarm.
   Distinguish call ordering from electrical propagation or stop latency.
5. Calculate the nominal centre pulse independently. Explain why a clean compile,
   a successful driver return and a physical waveform are three different kinds
   of evidence.

The host test uses a scripted S3-like shim, not the actual peripheral. The board
builds compile against real capability headers; neither build is hardware execution.

## Physical acceptance worksheet — NOT EXECUTED

Begin with motors unplugged and the power-cut path independently reviewed.
Record board/firmware identity, scope model, probes, ground arrangement and rails.

| Check | Required evidence |
| --- | --- |
| USB boot and camera startup, no arm | OE stays low; record both MCU and servo-side signal traces |
| Closed contact plus explicit arm | Measure frequency, pulse widths and first-pulse behavior on both outputs |
| Minimum/centre/maximum allowed commands | Measure commanded pulse range and jitter; do not infer shaft angle |
| Open auxiliary contact/disarm | Measure OE and output timing; compare separately with physical power cut |
| Power sequencing and reset | Record transients and back-feed checks from the electrical worksheet |
| Supported mechanism, no payload | Only after electrical approval: verify directions, travel and stopping |

All physical rows remain pending. No waveform, independent watchdog, torque-off,
balance or gravity-support guarantee is created by this firmware change.
