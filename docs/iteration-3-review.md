# Iteration 3 — candidate moving carriers and joint frames

2026-10-05. Untested mechanical draft; no motor operation approval.

## Delivered

- Parametric yaw deck/risers/pitch seat and pitch side-plate/camera shelf.
- Revised fixture with yaw-seat attachment holes and camera shelf holes aligned
  to the earlier saddle's slots. Both horn mounting lands remain undrilled.
- Eleven-solid STEP assembly, individual STEP/STL and editable parameter JSON.
- Right-handed yaw/pitch point transforms, explicit unmeasured joint origins,
  unknown optical frame and a recorded 25-pose intrusion review.
- Three BOM rows (25 total), README teaching and blank physical-measurement gates.

## Evidence

- Four standard-library Python frame tests pass; scoped Ruff checks pass.
- CAD checks pass for connected positive-volume solids and STEP roundtrips.
- 25 sampled combinations spanning nominal firmware offsets yaw±30/pitch±20:
  no detected positive-volume intrusion. Sampling checks actual candidate solids
  and coarse component envelopes; a positive control confirms intrusion detection.
- Point transforms agree with CAD envelope-centre transforms within 1e-6 mm.
- Firmware min/centre/max read from source and checked against sample endpoints.
- No firmware logic change, no new embedded/hardware execution claim. Python
  coordinate tests added to CI; CadQuery checks run locally, not lightweight CI.

## Still unfinished

Actual horns, shaft offsets, retention, opposite-side pitch bearing, load path,
wire routes, backlash, material strength, fasteners, optical pose and actual servo
travel remain unverified. The supplied servo body dimensions do not define its
shaft coordinate system. Sampled non-intrusion does not prove a clearance margin,
continuous sweep, pinch protection or stable camera tracking.

Next mechanical work requires real part measurements before attaching motors.
Further software work can independently improve detector/tracking input validation
and user calibration workflows without pretending these CAD assumptions are measured.
