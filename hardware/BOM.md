# Tracker Bot EVT-A.1 — bill of materials

Bench red-marker tracker; XIAO Sense + two PWM servos. MCU USB and motor 4.8 V rails are separate. No production approval.

Prices and procurement approval are not supplied. Quantities are per prototype.

| Ref | Qty | Selection / value | Selection status | Source |
| --- | ---: | --- | --- | --- |
| U1 | 1 | XIAO ESP32-S3 Sense | Selected camera/controller; dimensions are clearance envelope | [Seeed pin map](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/) |
| J1 | 1 | USB host / power cable | Prototype selection; bench verification required | [Design rationale](../docs/engineering.md) |
| P1 | 1 | Regulated 4.8 V motor supply | Set current limit 0.5 A initially; qualify up to 3 A | [Design rationale](../docs/engineering.md) |
| F1 | 1 | 2 A fuse + holder | Initial fuse value; coordinate with measured inrush | [Design rationale](../docs/engineering.md) |
| S1 | 1 | Latching power-cut switch, two isolated NC contacts | Power pole >=5 A DC at 5 V; aux pole rated for low-level signal; exact part TBD | [Design rationale](../docs/engineering.md) |
| U2 | 1 | TXU0102DCUR on verified VSSOP8 breakout | Dual-supply A-to-B translator; IC pin numbers, not breakout terminal numbers | [TI TXU0102 datasheet](https://www.ti.com/lit/ds/symlink/txu0102.pdf) |
| R1 | 1 | 10k input pull-down | Prototype selection; bench verification required | [Design rationale](../docs/engineering.md) |
| R2 | 1 | 10k input pull-down | Prototype selection; bench verification required | [Design rationale](../docs/engineering.md) |
| C1 | 1 | 100 nF ceramic / >=10 V | Prototype selection; bench verification required | [Design rationale](../docs/engineering.md) |
| C2 | 1 | 470 uF electrolytic / >=10 V | Observe polarity; near servo connectors | [Design rationale](../docs/engineering.md) |
| M1 | 1 | TowerPro SG90 yaw | Case envelope; shaft axis +Z, horn attachment TBD | [TowerPro SG90](https://towerpro.com.tw/product/sg90-analog/) |
| M2 | 1 | TowerPro SG90 pitch | Rotated case envelope; shaft axis +Y, bracket TBD | [TowerPro SG90](https://towerpro.com.tw/product/sg90-analog/) |
| R3 | 1 | 10k OE pull-down | Keeps translator disabled during MCU reset | [Electrical review](../docs/power-and-interlock.md) |
| R4 | 1 | 47k yaw PWM pull-down | Servo-side signal bias when translator output is high impedance | [Electrical review](../docs/power-and-interlock.md) |
| R5 | 1 | 47k pitch PWM pull-down | Servo-side signal bias when translator output is high impedance | [Electrical review](../docs/power-and-interlock.md) |
| R6 | 1 | 10k stop-contact pull-up | Open aux contact or broken wire -> logic high -> disarm | [Electrical review](../docs/power-and-interlock.md) |
| R7 | 1 | 1k 0.25W motor-rail bleeder | Nominal 23mW at 4.8V; discharge timing must be measured | [Electrical review](../docs/power-and-interlock.md) |
| C3 | 1 | 100nF ceramic / >=10V logic bypass | At TXU0102 pins 3 and 2 | [TI TXU0102 datasheet](https://www.ti.com/lit/ds/symlink/txu0102.pdf) |
| H3 | 1 | Draft yaw servo seat (parametric printed candidate) | NOT fabrication approved; retention, fit and plate attachment unresolved | [Mechanical draft guide](../mechanical/README.md) |
| H4 | 1 | Draft camera saddle (parametric printed candidate) | NOT fabrication approved; floating until pitch carrier is designed | [Mechanical draft guide](../mechanical/README.md) |
| H1 | 4 | M3 fixture screw + nut + washer set | For plate holes; not servo screws | [Design rationale](../docs/engineering.md) |
| H2 | 2 | Servo manufacturer horn and screw | Reuse supplied horn; do not guess spline or screw length | [Design rationale](../docs/engineering.md) |

## Net connections

Identical labels in the schematic are electrically connected.
NC means intentionally unconnected. Supply and connector ratings need physical verification.

- **USB_HOST**: U1.USB, J1.USB
- **GND**: U1.GND, J1.GND, P1.NEG, S1.AUX NC, U2.2 GND, R1.2, R2.2, C1.2, C2.NEG, M1.GND, M2.GND, R3.2, R4.2, R5.2, R7.2, C3.2
- **YAW_IN**: U1.D0 / GPIO1, U2.5 A1, R1.1
- **PITCH_IN**: U1.D1 / GPIO2, U2.4 A2, R2.1
- **LOGIC_3V3**: U1.3V3, U2.3 VCCA, R6.1, C3.1
- **PWM_ENABLE**: U1.D2 / GPIO3, U2.6 OE, R3.1
- **STOP_SENSE**: U1.D3 / GPIO4, S1.AUX COM, R6.2
- **MOTOR_RAW**: P1.POS, F1.IN
- **MOTOR_FUSED**: F1.OUT, S1.POWER COM
- **MOTOR_4V8**: S1.POWER NC, U2.7 VCCB, C1.1, C2.POS, M1.V+, M2.V+, R7.1
- **YAW_PWM**: U2.8 B1Y, M1.PWM, R4.1
- **PITCH_PWM**: U2.1 B2Y, M2.PWM, R5.1
