# Iteration 4 — capture freshness and camera input review

Date: 5 October 2026. Draft only; no hardware connected or flashed.

Corrected a stale-frame bug: camera targets formerly received a fresh timestamp
after retrieval. They now retain the camera's boot-relative capture-start time.
Only complete 160x120 RGB565 frames with valid, nonfuture timestamps within the
configured age limit reach marker processing. The controller independently
rejects stale/future targets at receipt. Queue and processing time do not renew
target lifetime. Conversion handles millisecond rollover and rejects malformed
large timestamp inputs without integer overflow.

Added last-frame classification, red sample count and capture age to `status`.
README, firmware guide and [calibration lesson](camera-calibration.md) distinguish
held PWM from physical stopping, and explicitly describe automatic reacquisition.
The physical camera/polarity/timing worksheet is deliberately unfilled.

## Verification

- Native production controller/servo/marker/frame-boundary tests: PASS, synthetic
  time and image data only. Added metadata, age-limit, future-time, rollover,
  red-count, color rejection and timestamp-preservation cases.
- Both pinned PlatformIO builds (`xiao-sense`, `esp32-s3-devkitc-1`): PASS.
- No camera DMA/scheduling, sensor color ordering, actual motor, physical stop or
  thermal validation. No CAD or electrical interface changes this iteration.
- Existing CI runs the same native test binary and both embedded builds; the
  new remote run must still be checked after push.

## Next engineering gates

1. Measure real sensor format/orientation/timing and actuator polarity before
   powered tracking; verify source timestamp behavior on the exact board.
2. Review serial parser failure semantics: rejected malformed target text still
   leaves the previous valid target eligible until expiry. Add parser-level tests
   before changing that behavior.
3. Add a scoped target selection strategy: current centroid combines red areas
   and does not maintain object identity. Measure distractors before claiming
   stable object tracking.
4. Resolve motor horns, positive component retention, bearing and cable sweep;
   then inspect and physically validate the previously drafted carrier CAD.

Verdict: stronger input boundary and clearer teaching material; not a finished
consumer tracker or independently safe motion system.
