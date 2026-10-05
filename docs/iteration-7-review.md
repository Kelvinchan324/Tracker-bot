# Iteration 7 — STEP-derived visual mechanical handoff

5 October 2026. Documentation/visualization refinement; no solid or firmware changes.

Added labelled isometric and side views generated from the actual candidate
STEP files and component-envelope manifest. Before drawing, all 11 bodies are
matched one-to-one against carrier-layout.step using volume/bounds signatures.
The renderer refuses a missing/mismatched body set rather than illustrating a
different assembly silently. This signature is not a topological identity proof.

Visual inspection clarified the stacked M1/M2 arrangement, offset pitch plate,
camera saddle and unfinished horn gaps. Corrected overlapping collapsed-depth
axis labels in the initial render. The final graphic names assumed joint frames
and explicitly labels optical calibration, horn attachment, body/board retention,
bearing and cable sweep as unfinished. Occlusion/tessellation can hide details;
use solids, not pixels, to review geometry. No physical fit is claimed.

Verification: renderer passed its 11-body matching gate; PNG visually inspected;
source/image freshness unittest and scoped Ruff pass. Existing geometry was not
regenerated or modified. Ledger records generator versions and source hashes;
CI adds a lightweight freshness check, not a full CAD build. Source text hashes
normalize CRLF to LF for cross-platform checkouts; STEP/PNG use native bytes.

The previous ambiguity-rejection firmware commit a16e333 passed remote Engineering
checks. This new revision's remote CI still requires checking after push. No
board flashed, motor operated or physical measurement acquired.
