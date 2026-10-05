# Tracker Bot EVT-A — bill of materials

Bench red-marker tracker; XIAO Sense + two PWM servos. MCU USB and motor 4.8 V rails are separate. No production approval.

Prices and procurement approval are not supplied. Quantities are per prototype.

| Ref | Qty | Selection / value | Selection status | Source |
| --- | ---: | --- | --- | --- |
| U1 | 1 | XIAO ESP32-S3 Sense | Selected camera/controller; dimensions are clearance envelope | [Seeed pin map](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/) |
| J1 | 1 | USB host / power cable | Prototype selection; bench verification required | [Design rationale](../docs/engineering.md) |
| P1 | 1 | Regulated 4.8 V motor supply | Set current limit 0.5 A initially; qualify up to 3 A | [Design rationale](../docs/engineering.md) |
| F1 | 1 | 2 A fuse + holder | Initial fuse value; coordinate with measured inrush | [Design rationale](../docs/engineering.md) |
| S1 | 1 | Latching DC power-cut switch | Contact DC rating >= 5 A at 5 V; not safety certified | [Design rationale](../docs/engineering.md) |
| U2 | 1 | SN74AHCT125N DIP14 buffer | 3.3 V input to 4.8 V servo PWM; DIP pin numbers | [TI SN74AHCT125](https://www.ti.com/product/SN74AHCT125) |
| R1 | 1 | 10k input pull-down | Prototype selection; bench verification required | [Design rationale](../docs/engineering.md) |
| R2 | 1 | 10k input pull-down | Prototype selection; bench verification required | [Design rationale](../docs/engineering.md) |
| C1 | 1 | 100 nF ceramic / >=10 V | Prototype selection; bench verification required | [Design rationale](../docs/engineering.md) |
| C2 | 1 | 470 uF electrolytic / >=10 V | Observe polarity; near servo connectors | [Design rationale](../docs/engineering.md) |
| M1 | 1 | TowerPro SG90 yaw | Case envelope; shaft axis +Z, horn attachment TBD | [TowerPro SG90](https://towerpro.com.tw/product/sg90-analog/) |
| M2 | 1 | TowerPro SG90 pitch | Rotated case envelope; shaft axis +Y, bracket TBD | [TowerPro SG90](https://towerpro.com.tw/product/sg90-analog/) |
| H1 | 4 | M3 fixture screw + nut + washer set | For plate holes; not servo screws | [Design rationale](../docs/engineering.md) |
| H2 | 2 | Servo manufacturer horn and screw | Reuse supplied horn; do not guess spline or screw length | [Design rationale](../docs/engineering.md) |

## Net connections

Identical labels in the schematic are electrically connected.
NC means intentionally unconnected. Supply and connector ratings need physical verification.

- **USB_HOST**: U1.USB, J1.USB
- **GND**: U1.GND, J1.GND, P1.NEG, U2.7 GND, U2.1 /1OE, U2.4 /2OE, U2.9 3A, U2.12 4A, R1.2, R2.2, C1.2, C2.NEG, M1.GND, M2.GND
- **YAW_IN**: U1.D0 / GPIO1, U2.2 1A, R1.1
- **PITCH_IN**: U1.D1 / GPIO2, U2.5 2A, R2.1
- **MOTOR_RAW**: P1.POS, F1.IN
- **MOTOR_FUSED**: F1.OUT, S1.COM
- **MOTOR_4V8**: S1.NC, U2.14 VCC, U2.10 /3OE, U2.13 /4OE, C1.1, C2.POS, M1.V+, M2.V+
- **YAW_PWM**: U2.3 1Y, M1.PWM
- **PITCH_PWM**: U2.6 2Y, M2.PWM
