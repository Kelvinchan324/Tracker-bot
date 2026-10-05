# Iteration 9 — ESP32-S3 PWM initialization blocker

5 October 2026. No hardware connected or flashed.

Source audit found a runtime blocker missed by previous successful builds: the
firmware requested 16-bit LEDC resolution, while ESP32-S3's capability header
allows at most 14 bits. The pinned driver returns zero on that unsupported
request; the application and its old void-returning native shim ignored it.

Changed to 14-bit/50 Hz with embedded capability assertions. Each axis requires
the driver's expected frequency before it is ready. Either failed axis blocks
arming of both; startup and status expose failure. No automatic retry exists.
Zero duty is set before attaching pins, both pulse targets precede OE high, and
OE goes low before disarm clears PWM. Reinitialization resets command/contact
state. Pulse conversion uses 2^14 period counts and nearest-count rounding.

Verification: all five strict-warning native programs and five Python tests pass.
The new actual-main-loop PWM test injects failed/unexpected setup results, checks
ordering and diagnostics, and sweeps pulse arithmetic independently. Real board
builds both succeed: XIAO Sense RAM 37,508 bytes, flash 331,361 bytes; DevKit RAM
19,200 bytes, flash 293,709 bytes. CI now includes the new native program.

Source references, a teaching exercise and unexecuted scope worksheet are in
[PWM startup](pwm-startup.md). The hardware manifest, pins, CAD, dependency pin,
motion limits and stop power path are unchanged. Physical attach/write success,
timer accuracy, transients, actual servo pulse widths and stop time remain
unverified. This fixes an evidenced software configuration error, not a hardware
acceptance gate. Prior test records remain historical and must not be read as
proof that their 16-bit firmware generated valid PWM.
