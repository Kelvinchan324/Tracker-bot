# Serial controls and input-failure lesson

Revision: 5 October 2026. Development firmware, not a safety-rated controller.
These behaviors have native software tests and embedded compile evidence, not
physical servo, USB timing or stopping measurements.

## Protocol

Use the 115200-baud serial monitor with newline termination. Commands are ASCII,
case-insensitive, and separated from arguments by spaces or tabs. A line has at
most 96 bytes before CR/LF. CRLF is one effective terminator, not two commands.
Empty lines do nothing. Complete a line within 1,000 ms of its first byte being
processed; the exact boundary is allowed, longer delays are rejected. Send whole
lines from the serial monitor rather than slowly typing raw terminal characters.

| Command | Meaning and preconditions |
| --- | --- |
| `arm` | Closed auxiliary stop contact required; applies stored position and may move abruptly |
| `disarm` | Exit camera mode; disable PWM/OE; payload can fall; does not physically isolate power |
| `camera` | Select initialized red-marker input only with `camera_stream=live`; does not arm |
| `serial` | Exit camera tracking and stop increments |
| `target x y` | Exactly two finite decimal/scientific numbers in [-1,1]; armed serial mode only |
| `center` | Exit camera mode and command center when armed; not a gradual trajectory |
| `stop` | Exit camera mode and stop new increments, holding the last PWM |
| `status` / `help` | Read-only command handling; do not renew target lifetime |

Examples: `target .2 -1e-1` and `TARGET 0.2 -0.1` are valid equivalents.
`target0 0`, `target nan 0`, `target 0x1p-1 0`, `target 0,2 0`, `arm extra`
and `target 0 0 extra` are invalid. Values are parsed as numbers and converted
to controller floats; this is not an exact decimal or precision-motion protocol.

## What changed

Previously a malformed target printed an error but left the previous target
eligible until its normal timeout. The new parser and command dispatcher stop
tracking on any unknown/malformed nonempty command, and leave camera mode so a
camera frame cannot immediately restart motion. The arming state and last PWM
are retained. If a valid target is sent while disarmed or in camera mode, it is
rejected and tracking stops; select the intended mode and retry deliberately.

Invalid control bytes (including embedded NUL), non-ASCII bytes and an oversized
line are rejected as soon as processed. An incomplete line expires after one
second, even when no further bytes arrive, provided the foreground loop runs.
The rest of a rejected line is discarded through the next terminator: a suffix
such as `arm` is never interpreted as a replacement command from that same line.
To recover, send a newline, then a fresh complete command. Recovery never arms
automatically. A subsequent complete explicit `arm` is still an operator action.

The line buffer uses fixed storage, not a growing Arduino String. The main loop
still consumes at most 128 serial bytes per pass and rechecks the stop contact
at command dispatch. Opening that contact disarms even if the command is only
`status`; closing it alone does not re-arm. These checks depend on a functioning
foreground loop and are not hardware-latched fault supervision.

## Teaching sequence — start with motors disconnected

1. Run the native parser/controller tests from [tests/README.md](../tests/README.md).
   Explain why strict parsing should happen before any arming action.
2. With motor power physically disconnected and the board's normal reviewed
   low-voltage wiring, observe the serial responses to `arm extra`, `target nan 0`
   and `help`. Do not bridge or defeat the physical stop contact for this lesson.
3. In the native test, trace `arm -> target .5 0 -> target nan 0`: tracking ends,
   but PWM remains enabled. Compare `disarm`. Explain why neither state name
   proves a supported mechanism is safe to touch.
4. Trace a 97-byte line followed by `arm` before its newline: the suffix is
   discarded. Trace a partial `ar`, a delay over one second, then `m`: it must not
   produce a complete arm command. Finally send newline and a fresh command.
5. Before physical motion tests, use the existing supported, current-limited,
   no-payload procedure and independently record signal behavior. No powered
   test is required to complete the software lesson.

## Physical worksheet — not executed

| Evidence | Result |
| --- | --- |
| Operator/observer, exact board, firmware commit, monitor/newline setting | pending |
| Physical motor isolation and restraint before input tests | pending |
| Malformed command response and absence of unintended new target | pending |
| 96/97-byte boundary and discarded-suffix transcript | pending |
| Partial-line expiry, recovery and observed latency | pending |
| PWM hold versus PWM disable, measured separately from mechanical motion | pending |
| Stop-contact precedence and no automatic re-arm | pending |
| Sustained serial input, loop latency and input-buffer overflow behavior | pending |

No authentication, sequence numbers or host-origin timestamps are provided.
This is a local educational serial interface, not a network motion-control API.
USB/UART driver overflow, serial-output blocking, electrical glitches and a hung
CPU remain physical/integration review items; the independent power cut is required.
