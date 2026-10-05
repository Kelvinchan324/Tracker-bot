# Iteration 8 — explicit camera-stream fault recovery

5 October 2026. Firmware/software evidence only; no hardware attached.

Previously target freshness stopped increments during camera silence, but camera
mode remained selected and fresh frames could restart motion automatically.
Boot initialization also counted as camera readiness indefinitely. Added a
main-loop stream monitor that uses the original capture timestamp, not delivery
time, and differentiates ordinary marker loss from unusable image input.

- Initialization without a fresh valid image no longer permits camera selection.
- Fresh no-marker/ambiguous images keep the stream live and stop increments;
  ordinary marker reacquisition remains automatic while camera mode is selected.
- Invalid/stale/no-frame results or capture age over 750 ms stop increments and
  exit camera mode. PWM/arming are retained, so support the mechanism and use the
  independent physical power cut when necessary.
- Fresh frames after the fault update diagnostics only. Inspect the cause, then
  explicitly send `camera` to select the recovered stream; this does not arm.
  A later delivered valid marker is required for renewed tracking (or the fresh
  frame received in the same loop as the explicit command).
- `camera_stream` complements historical `last_frame` diagnostics. A single mode-
  exit message is emitted per fault, not on every loop. Serial targets remain
  independent of camera-stream health.

The new native test includes production `main.cpp`, real controller/servo code,
and scripted serial/camera/time/GPIO boundaries. It tests initial readiness,
exact 750/751 ms freshness, future/old receipt timestamps, silent camera input,
invalid/stale/no-frame results, explicit recovery, normal marker/ambiguity loss,
PWM hold, command ordering, stop contact, disarmed selection and uint32 rollover.
The host shim does not simulate FreeRTOS, electrical drivers or wall-clock timing.
CI now builds and runs this strict-warning C++17 test.

Verification: new main-loop test and existing controller/serial/component tests
pass. Python frame and visual-review checks pass. Both PlatformIO targets compile:
XIAO Sense RAM 37,508 bytes / flash 330,849 bytes; DevKitC RAM 19,200 / flash 293,197.
These sizes are build reports, not free-runtime-heap or stack measurements. New
remote CI result was not yet checked at commit time. No dependency or CAD change.

This is not an independent watchdog. The main loop must continue executing;
blocked serial output, frozen MCU, real queue scheduling, repeated/faulty camera
timestamps and actual stopping latency require separate hardware validation.
No task restart, power removal, encoder feedback or obstacle detection is added.
Held PWM is neither a physical stop nor guaranteed torque behavior. Do not
hot-unplug a powered camera flex cable to test recovery. The teaching manual
provides synthetic fault exercises and retains the unfilled physical worksheet.
