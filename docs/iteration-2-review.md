# Engineering review — iteration 2, 5 October 2026

Disposition: **continue; electrical and mechanical drafts are not hardware-approved**.

## Changes

- Replaced AHCT125 with dual-supply TXU0102 and explicit OE control/pull-down.
  Selection is based on TI's specified partial-power-down behavior, not bench evidence.
- Added two isolated NC power-cut contacts: motor power interruption and a
  low-level auxiliary contact. Opening the auxiliary input disarms; closing alone
  does not re-arm. Broken-open wiring inhibits arm.
- Added servo-side pull-downs, logic bypass capacitor and motor-rail bleeder.
- Bounded serial processing so input cannot occupy an unbounded loop iteration.
- Updated schematic, 22-row BOM, six-envelope layout, GPIO assignments and manuals.
- Added editable yaw-seat and camera-saddle CAD with slotted mounting feet and a
  common coordinate frame. Candidate parts still need positive retention, measured
  interfaces and the missing rotating/pitch carriers.

## Evidence

| Check | Result |
| --- | --- |
| Native production-controller logic | PASS, including contact-open inhibit, OE disable, no automatic re-arm |
| XIAO Sense firmware | Compile PASS; RAM 22,964 bytes, flash 350,309 bytes |
| DevKit firmware | Compile PASS; RAM 19,056 bytes, flash 313,301 bytes |
| Layout CAD | Seven-solid STEP round-trip PASS |
| Two candidate mounts | Each valid connected solid, positive volume, no assumed-envelope intrusion, STEP re-import PASS |
| Combined mounting STEP | Four bodies retained (two parts and two reference envelopes) |

No physical stop time, back-feed, startup transient, thermal result, printed fit,
load capacity, moving collision check or servo performance was measured.

## Known limitations and next steps

1. MCU polling is not a safety controller. A welded contact is not diagnosed;
   independent motor-supply loss/return is not sensed. Bench review must verify
   that the power pole actually interrupts energy, with supported loads.
2. Exact TXU breakout, switch/contact ratings and connector procurement remain open.
3. Complete a rotating yaw-to-pitch carrier using measured horn axes/fasteners;
   camera saddle is currently floating. Match mounting-foot patterns to the base.
4. Inspect lens/antenna/USB keepouts and cable travel from real measurements.
5. Run and record every condition in the power/interlock worksheet. Do not promote
   mathematical RC estimates or host tests into measured hardware claims.
