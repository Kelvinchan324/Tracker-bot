# Iteration 6 — reject spatially ambiguous red-marker frames

5 October 2026. Software-tested engineering draft; motors/camera untested.

The old detector pooled every red sample into one centroid. It could aim between
two separate red objects, or accept scattered noise totaling 20 samples. The new
bounded flood fill requires exactly one four-connected region with at least
20 samples on the every-other-row/column grid. Subthreshold disconnected regions
are excluded from the accepted centroid. Multiple candidates produce `ambiguous`,
clear coordinates and use the existing no-target stop-increments path on delivery.
Capture timestamps, arming gates and command limits are unchanged.

The camera owns a 14,400-byte static workspace (4,800 bytes mask + 9,600 bytes
queue), not a large automatic array on the 4,096-byte capture-task stack. Every
sample is enqueued at most once; no recursion/heap allocation. Detector supports
up to 160x120; capture still requires exact QQVGA. New status fields distinguish
all red samples, qualifying candidates and samples in the unique accepted region.

Verification: new strict-warning component test and existing control/camera and
serial tests pass. Cases include separate candidates, unequal candidate sizes,
split subthreshold regions, speckles, diagonal/bridged adjacency, edge coordinates,
odd-size input, 4,800-sample capacity, canaries and workspace reuse. Both embedded
targets compile: XIAO RAM37,492/flash330,477; DevKit RAM19,184/flash292,841.
New test added to engineering CI; remote result must be checked after push.

Important remaining limitations: one connected red background or distractor can
qualify; touching regions merge; no temporal identity selection or upper-area
rejection. Automatic reacquisition still applies. Thresholds, byte order,
latency/heap/stack headroom, mechanical retention and stopping remain physically
unverified. PWM holds last command on rejected frames; this is not a motor power
cut. README and calibration lesson include a motors-disconnected exercise.
