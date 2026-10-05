# Software checks (no hardware attached)

Compile the production controller and servo sources against the tiny GPIO/timer
shim in `native/`; this tests logic, not actual PWM signals or motor behavior.
With a C++17 compiler, from the repository root:

```sh
c++ -std=c++17 -Itests/native -Ifirmware/include tests/native/test_control.cpp firmware/src/tracker_controller.cpp firmware/src/servo_axis.cpp -o /tmp/tracker-tests
/tmp/tracker-tests
```

On Windows, `python -m pip install ziglang==0.13.0` provides an optional compiler.
Replace `c++` with `python -m ziglang c++` and choose an output path under an ignored
`build/` directory. Embedded compilation is separate: `pio run -d firmware`.
Neither test substitutes for the staged electrical and mechanical acceptance
procedure in `docs/engineering.md`.

The same native binary tests the production `camera_frame.h` boundary: exact
RGB565 dimensions/length, capture-age boundaries, malformed/future timestamps,
overflow-resistant conversion, millisecond rollover, red sample threshold,
green/white rejection and original capture timestamp preservation into control.
Camera metadata and GPIO/time are synthetic; FreeRTOS scheduling, actual DMA
buffers, byte order, sensor output and PWM are not exercised on hardware.

Run the production serial parser, fixed line buffer and command effects against
the real controller/servo sources and host shim:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -Itests/native -Ifirmware/include tests/native/test_serial.cpp firmware/src/tracker_controller.cpp firmware/src/servo_axis.cpp -o /tmp/serial-tests
/tmp/serial-tests
```

Cases include strict argument counts/numeric grammar, invalid-input cancellation,
wrong input mode, interlock precedence, PWM hold versus disable, 96/97-byte
boundaries, NUL/non-ASCII rejection, CRLF, timeout/recovery and clock rollover.
The full UART/USB driver and main-loop scheduling are not simulated by this test.

Coordinate math tests (standard-library Python, also in CI):
`python -m unittest discover -s tests -p test_frames.py`. These check right-handed
axes, pivot/distance preservation, pitch-before-yaw composition and invalid inputs.
The optional CadQuery generator `python tools/build_carriers.py` performs local
25-pose solid/envelope intrusion checks; it is not part of lightweight CI and
does not establish continuous clearance or physical calibration.
