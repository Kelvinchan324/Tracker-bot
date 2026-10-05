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
