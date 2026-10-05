# EVT-A.1 electrical review and acceptance worksheet

Status: engineering draft; no measured power-off or motor result exists.
The revision manifest generates the authoritative pin/net drawing and BOM.
The previous AHCT125 DIP14 arrangement is superseded, not pin-compatible.

## Source basis and design decision

TI's [TXU0102 SCES941A](https://www.ti.com/lit/ds/symlink/txu0102.pdf)
specifies independently powered 1.1–5.5 V ports, partial-power-down support,
and high-impedance outputs when either supply is below 100 mV or disconnected.
OE low also disables both outputs. The DCU/VSSOP8 pin table is:
1 B2Y, 2 GND, 3 VCCA, 4 A2, 5 A1, 6 OE, 7 VCCB, 8 B1Y.
This supports the design selection; it does not validate a purchased breakout.

Design interpretation: motor and logic rails can be sequenced separately without
relying on an ordinary buffer's unspecified output behavior during power-down.
Shared ground remains necessary. This is not a galvanic barrier, safe-torque-off
device, or certified emergency-stop architecture.

## Net-level assembly schedule

| Harness / interface | Connections | Construction/check |
| --- | --- | --- |
| Motor source | P1 + -> F1 -> S1 power NC -> MOTOR_4V8 | Insulated rated wire/connectors; external current-limited supply |
| Motor returns | Both servos -> supply GND star | Do not return servo current through MCU or breadboard |
| Logic rail | XIAO 3V3 -> U2 pin 3; common GND -> pin 2 | C3 100 nF directly at pins |
| Translator motor rail | MOTOR_4V8 -> U2 pin 7 | C1 100 nF locally; C2 470 µF near motor connectors |
| Yaw signal | GPIO1 -> A1/5; B1Y/8 -> M1 PWM | R1 10k input pull-down, R4 47k servo-side pull-down |
| Pitch signal | GPIO2 -> A2/4; B2Y/1 -> M2 PWM | R2 10k input pull-down, R5 47k servo-side pull-down |
| Output enable | GPIO3 -> OE/6 | R3 10k to GND, default disabled |
| Stop auxiliary | GPIO4 -> S1 isolated AUX NC -> GND | R6 10k to logic 3.3 V; never connect motor voltage to this pole |
| Motor discharge | MOTOR_4V8 -> R7 1k -> GND | 0.25 W resistor; nominal 23 mW dissipation at 4.8 V |

GPIO numbering above is XIAO only. DevKit uses PWM4/5, OE6, stop7.
Choose S1 with two electrically isolated, mechanically linked NC contacts:
one with suitable DC motor-current interruption rating, the other specified for
low-level signal service. The exact switch, wire gauge and keyed connector model
remain procurement gates. Do not assume a generic two-terminal button suffices.

At nominal values, the disconnected 470 µF capacitor and 1k bleeder have
RC=0.47 s; 4.8 V to 0.1 V takes about 1.82 s with no other load. Stored energy
is about 5.4 mJ. These are calculations, not measured stop time. Component
tolerance and actual loads change the result; rail interruption is not instantaneous.

## Software behavior and limitations

The OE output is held low during startup. Closing the contact permits a deliberate
arm; opening it disables OE and PWM on the next control-loop observation.
Reclosing cannot arm the controller. Serial processing is limited to 128 bytes
per pass to avoid unbounded input processing.

No claim is made about maximum stop latency. A hung MCU cannot be trusted to
sample the auxiliary pole; the separate power pole must interrupt the motor rail.
A welded or bypassed auxiliary contact cannot be diagnosed by this single channel.
An unrelated loss/return of motor supply is not measured and may restore a prior
command: **disarm and support the mechanism before connecting any supply**.
Boot transients, servo behavior without pulses and pole timing need oscilloscope
and restrained-mechanism tests. A servo can drop its load when torque disappears.

## Required recorded tests

Do the signal tests with motors unplugged first. Record exact component identities,
firmware commit, instruments, wiring photographs and measured waveforms.

| Condition / exercise | Intended behavior | Actual evidence |
| --- | --- | --- |
| USB only, motor supply off | No sustained servo-output drive/back-fed motor rail | Not tested |
| Motor only, USB off | Outputs unpowered/high impedance, servo-side pull-downs active | Not tested |
| Both supplies, boot | OE low; no command until arm and contact closed | Not tested |
| Aux disconnected at boot | Arm refused | Not tested |
| Arm, then press stop | Power pole opens; software disarms on observed open aux | Not tested |
| Release stop | Still disarmed until a new deliberate arm | Not tested |
| Continuous serial traffic | Stop still sampled; record latency | Not tested |
| Rail discharge after cut | Measure curve; compare with nominal RC estimate | Not tested |
| Restart with supported mechanism | No unexpected resumed camera target | Not tested |
| Restrained motors, worst allowed load | Current, temperature, travel and stop behavior recorded | Not tested |

Do not call the electrical design finished until every relevant state has actual
evidence and an independent wiring/mechanical review.
