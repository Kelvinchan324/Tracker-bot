# Mechanical drafts: what can and cannot be built

The layout STEP locates electronics and actuators as **bounding envelopes**.
It is not a full moving assembly. Two additional parametric candidates now exist:

- `yaw-seat-draft.step/.stl`: servo body seat, cable relief and slotted mounting feet.
- `camera-saddle-draft.step/.stl`: open-ended board saddle with slotted mounting feet.
- `mounting-drafts.step`: both candidates in the same world coordinates as their
  component envelopes. Their separated locations are intentional; no connecting
  mechanism is modeled yet.

Run `python tools/build_mounts.py` from the repository root with CadQuery 2.8.0.
Edit `mount-parameters.json` for walls, fit allowance and slots; component sizes
and positions come from `hardware/rev-a.json`. Do not manually move the exports
without updating the source. `mount-checks.json` records attachment-slot centres
and computational volume/fit checks.

## Interface status

| Interface | Current definition | Gate before fabrication/operation |
| --- | --- | --- |
| M1 yaw body | 23 x 12.2 x 29 mm source envelope; +Z intended shaft direction | Measure ears, cable exit, true shaft offset and clamp fit |
| Servo seat | 0.6 mm per-side allowance, 3 mm walls, 2.5 mm base, 10 mm wall height | Printing tolerance, positive retention and strength tests |
| M2 pitch body | Rotated case envelope; +Y intended shaft direction | Actual mounting ears, horn axis and load path unresolved |
| Camera board | 22 x 18 x 15 mm assumed clearance envelope at (15,0,87) | Exact board/camera stack, antenna, lens and USB access |
| Camera saddle | Open ends, 8 mm walls, slotted feet | Must not contact antenna or block field of view |
| Mount foot slots | 6 mm total length, 3.4 mm width, elongated along Y | Matching holes/backing access, screw length and edge strength |
| Yaw-to-pitch carrier | NOT MODELED | Design around supplied horns and measured centres |
| Pitch-to-camera joint | NOT MODELED | Supplied horn attachment, opposite-side bearing/load review |

The yaw seat bottom is at the fixture plate top (4 mm). The camera saddle is
positioned under the camera envelope, but it is floating until its carrier is
designed. Plate exports do not yet contain the new mounting-foot patterns.
Do not treat floating CAD or zero envelope interference as a functioning mechanism.

## Measurement lesson (60 minutes)

1. Record servo body, ears, screw centres, horn plane, shaft offset and cable exit
   in millimetres. Photograph the calliper setup and note uncertainty.
2. Record camera stack, connectors, antenna keepout and optical axis.
3. Update source envelopes and regenerate candidates; compare against measurements.
4. Check fastener insertion, cable bend, pinch points and the entire intended
   yaw/pitch travel. The software angle labels are not measured shaft angles.
5. Obtain a mechanical review before printing load-bearing parts or powering motors.

CAD validation currently checks connected positive-volume solids, STEP re-import
and no penetration into the assumed static component envelope. It does not check
stress, print orientation, inserts, tolerances, moving collisions or fatigue.
