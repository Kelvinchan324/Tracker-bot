# Engineering review — iteration 1, 5 October 2026

Disposition: **continue; not a finished or hardware-validated product**.

## Delivered

- XIAO Sense red-marker capture task connected to the two-axis controller.
- Explicit arming, PWM disable, finite input validation, bounded control interval,
  stale-target timeout, serial mode and status feedback.
- 14-row component BOM, nine-net module schematic, six component envelopes,
  placement drawing, editable manifest, drilled fixture STEP/STL and layout STEP.
- README quick start, detailed build/manual and four hands-on lessons.

## Evidence actually obtained

- PlatformIO 6.2.0 / Espressif32 6.10.0: XIAO Sense and DevKitC builds PASS.
- Native C++ production-controller tests with mocked GPIO/time: PASS for boot
  disarmed, enable/disable, travel limits, deadband, finite input rejection,
  750 ms expiry, 40 ms catch-up cap, timer rollover and RGB565 marker detection.
- CadQuery 2.8.0: valid positive-volume fixture; layout STEP round-trip seven solids.
- Manifest validates unique references, sourced selections, geometry and connected nets.
- CI workflow added; remote workflow outcome must be checked after push.

No motor, camera, power-cut, current, temperature or printed part was tested.

## Next iteration, in priority order

1. Resolve independent signal isolation on power-cut; validate power-off states
   against component datasheets before presenting wiring as a build release.
2. Design parameterized yaw/pitch brackets with explicit measured-interface gates;
   separate placeholders from fabrication-ready geometry. Record horn-axis offsets.
3. Add electrical load budget, harness/connector schedule and recorded bench worksheets.
4. Exercise serial parser and camera failures; add image fixtures and polarity checks.
5. Physical owner: measure horns/holes, scope signals, verify limits/power-cut, run
   restrained marker trials and endurance test. This step cannot be simulated away.
