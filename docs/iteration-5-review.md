# Iteration 5 — strict serial command boundary

5 October 2026. Software-tested engineering draft; no board flashed or motors run.

Replaced loosely parsed serial text and growing String buffering with a fixed
96-byte line buffer, strict shared parser and testable command dispatcher.
Unknown/malformed commands now stop incremental tracking and leave camera mode;
they cannot silently retain the previous active target. Wrong-mode targets are
explicitly rejected. PWM remains held until disarm or the separate hardware
power cut; no physical safety guarantee is added.

Framing rejects NUL/non-ASCII/overflow immediately, expires partial lines beyond
one second and discards the remainder through CR/LF. Commands accept exact token
counts and case-insensitive words; decimal/scientific values only. Interlock is
rechecked at dispatch. Read-only commands never refresh the target timestamp.

Verification: new native serial/parser/framer tests pass under strict compiler
warnings using production controller and servo sources. Both pinned embedded
targets compile successfully. Existing controller/camera native checks also pass.
The new binary is included in engineering CI; remote result must be
checked after push. No claim of UART/USB scheduling, physical PWM or stop timing.

README and [serial lesson](serial-controls.md) include exact rejection semantics,
recovery sequence and a blank physical worksheet. This closes the prior review's
malformed-target retention item. Remaining: target identity/distractor handling,
measured mechanical retention and cable sweep, supported hardware tests, and
independent fault supervision.
