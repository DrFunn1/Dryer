# Dryer Eurorack Module - Wiring Diagram

## Complete Hardware Schematic

```
════════════════════════════════════════════════════════════════════════════════
                    RASPBERRY PI ZERO 2W PINOUT (GPIO)
════════════════════════════════════════════════════════════════════════════════

         3.3V  (1) ●●  (2)  5V          ┌─────────────┐
    I2C  SDA  (3) ●●  (4)  5V           │ Raspberry   │
    I2C  SCL  (5) ●●  (6)  GND          │  Pi Zero    │
     GPIO  4  (7) ●●  (8)  GPIO 14 TX   │  2W / W     │
          GND  (9) ●● (10)  GPIO 15 RX  │             │
     GPIO 17 (11) ●● (12)  GPIO 18      │  [HDMI]     │
     GPIO 27 (13) ●● (14)  GND          │             │
     GPIO 22 (15) ●● (16)  GPIO 23      │  [USB] [USB]│
         3.3V (17) ●● (18)  GPIO 24      └─────────────┘
     GPIO 10 (19) ●● (20)  GND
     GPIO  9 (21) ●● (22)  GPIO 25
     GPIO 11 (23) ●● (24)  GPIO 8
          GND (25) ●● (26)  GPIO 7
         ...  (27-40 not used for basic setup)

════════════════════════════════════════════════════════════════════════════════
```

## 1. ADS1115 ADC Module Connection

```
┌─────────────────┐                 ┌──────────────────┐
│   ADS1115 ADC   │                 │  Raspberry Pi    │
│   16-bit I2C    │                 │    Zero 2W       │
├─────────────────┤                 ├──────────────────┤
│                 │                 │                  │
│  VDD ●─────────────────────────────● Pin 1 (3.3V)    │
│  GND ●─────────────────────────────● Pin 6 (GND)     │
│  SCL ●─────────────────────────────● Pin 5 (GPIO 3)  │
│  SDA ●─────────────────────────────● Pin 3 (GPIO 2)  │
│                 │                 │                  │
│  ADDR ●─────────┐                 │                  │
│  ALRT ●         │ (not used)      │                  │
│                 │                 │                  │
│  A0  ●──────────┼─────► POT 1 (RPM)                  │
│  A1  ●──────────┼─────► POT 2 (Drum Size)            │
│  A2  ●──────────┼─────► POT 3 (Vanes)                │
│  A3  ●──────────┼─────► POT 4 (Vane Height)          │
│                 │                 │                  │
└─────────────────┘                 └──────────────────┘

ADDRESS: 0x48 (ADDR pin → GND)
```

## 2. Potentiometer Wiring (All 4 Identical)

```
       3.3V
        │
        │
        ├───────┐
        │       │
       ┌┴┐      │
       │ │      │ 10kΩ Linear Pot
       │ │      │
       │ │◄─────┘ (Rotation: 0-270°)
       │ │
       │ │
       └┬┘
        │
        ├──────► To ADS1115 (A0, A1, A2, or A3)
        │
       ┌┴┐
       │ │
       │ │
       └┬┘
        │
       GND

VOLTAGE RANGE: 0V (CCW) → 3.3V (CW)
ADC READING:   0 (min)  → 26400 (max, 16-bit)
```

## 3. Toggle Switches (3 identical circuits)

```
                    3.3V
                     │
                     │
                    ╱○  SPST Toggle Switch
                   ╱
                  ○
                  │
                  ├──────► GPIO 17 (Ball Type)
                  │        GPIO 27 (Lint Trap)
                  │        GPIO 22 (Moon Gravity)
                  │
                 [Pi internal pull-down resistor]
                  │
                 GND

LOGIC: 0 = Switch OFF (open)
       1 = Switch ON (closed, 3.3V applied)
```

## 4. MIDI Output Circuit (TRS Type A)

```
                        ┌──────────────┐
                        │ Raspberry Pi │
                        │  GPIO 14 TX  │
                        └──────┬───────┘
                               │ 3.3V UART TX
                               │
                          220Ω │
                         ┌─────┴─────┐
                         │           │
                         │          220Ω
                         │           │
                    Pin 4 (Tip)  Pin 5 (Ring)
                         │           │
                         └─────┬─────┘
                               │
                               │
                          Pin 2 (Sleeve)
                               │
                              GND

TRS JACK (Female, Panel Mount):
  Tip    (4) → MIDI Data (+)
  Ring   (5) → MIDI Data (-)
  Sleeve (2) → Ground

NOTE: For Type B MIDI, swap Tip and Ring connections

BAUD RATE: 31,250 (or 38,400 if 31.25k not available)
DATA: 8N1 (8 bits, no parity, 1 stop bit)
```

## 5. Eurorack Trigger Outputs (2 identical circuits)

```
                        ┌──────────────┐
                        │ Raspberry Pi │
                        │  GPIO 23/24  │
                        └──────┬───────┘
                               │ 3.3V Logic
                               │
                              1kΩ
                               │
                     ┌─────────┴──────────┐
                     │                    │
                     │  2N2222 NPN        │
                     │    (TO-92)         │
                     │                    │
                     │      C             │
                     │      │             │
                     │    ┌─┴─┐           │
                Base ├────┤   │           │  +5V from
                 (B) │    └─┬─┘           │  eurorack
                     │      │ E           │  power supply
                     │      │             │      │
                     │     GND            │     1kΩ
                     │                    │      │
                     └────────────────────┴──────┴──► TO JACK TIP
                                                  │
                                                 GND

FUNCTION: Converts 3.3V GPIO → 0-5V trigger pulse
DURATION: 10ms (configurable in pins.h)
CURRENT: ~5mA (limited by 1kΩ resistor)

JACK WIRING:
  Tip    → Collector (trigger signal)
  Sleeve → Ground
```

## 6. Display Connection (480x480 Round HDMI)

```
┌───────────────────┐          ┌──────────────────┐
│  Round Display    │          │  Raspberry Pi    │
│    480 x 480      │          │                  │
│                   │   HDMI   │                  │
│   [  Screen  ]    │──────────│   [HDMI Out]     │
│                   │  Cable   │                  │
│   5V Power Input  │          │                  │
│        │          │          │                  │
│        └──────────┼──────────┤  5V Power Rail   │
│                   │          │                  │
└───────────────────┘          └──────────────────┘

CONFIG (/boot/config.txt):
  hdmi_group=2
  hdmi_mode=87
  hdmi_cvt=480 480 60 1 0 0 0
  hdmi_force_hotplug=1
```

## 7. Power Distribution

```
                    ┌─────────────────────┐
                    │  5V Power Supply    │
                    │  (2A minimum)       │
                    └──────────┬──────────┘
                               │ 5V
                ┌──────────────┼──────────────┐
                │              │              │
                │              │              │
           ┌────┴────┐    ┌────┴────┐   ┌────┴────┐
           │   Pi    │    │ Display │   │ ADS1115 │
           │ (via    │    │ (direct │   │  (via   │
           │  USB)   │    │  5V in) │   │ 3.3V LDO│
           └─────────┘    └─────────┘   └─────────┘

CURRENT DRAW:
  Pi Zero 2W:  ~400mA (peak ~700mA)
  Display:     ~200mA
  ADS1115:     ~1mA
  Total:       ~600-1000mA
  
RECOMMENDED: 2A (2000mA) power supply for safety margin
```

## 8. Complete System Wiring Checklist

### I2C Bus (ADS1115)
- [ ] VDD → Pi Pin 1 (3.3V)
- [ ] GND → Pi Pin 6 (GND)
- [ ] SCL → Pi Pin 5 (GPIO 3)
- [ ] SDA → Pi Pin 3 (GPIO 2)
- [ ] ADDR → GND (for 0x48 address)

### Potentiometers (4x)
- [ ] POT1: GND - Wiper(A0) - 3.3V → RPM
- [ ] POT2: GND - Wiper(A1) - 3.3V → Drum Size
- [ ] POT3: GND - Wiper(A2) - 3.3V → Vanes
- [ ] POT4: GND - Wiper(A3) - 3.3V → Vane Height

### Toggle Switches (3x)
- [ ] SW1: 3.3V ─╱○ GPIO 17 (Ball Type)
- [ ] SW2: 3.3V ─╱○ GPIO 27 (Lint Trap)
- [ ] SW3: 3.3V ─╱○ GPIO 22 (Moon Gravity)

### MIDI Output
- [ ] GPIO 14 TX → 220Ω → TRS Tip (4)
- [ ] GPIO 14 TX → 220Ω → TRS Ring (5)
- [ ] Pi GND → TRS Sleeve (2)

### CV Trigger Outputs (2x)
- [ ] GPIO 23 → 1kΩ → 2N2222 Base → Collector to Jack
- [ ] GPIO 24 → 1kΩ → 2N2222 Base → Collector to Jack
- [ ] +5V (eurorack) → 1kΩ → Collectors
- [ ] Emitters → GND

### Display
- [ ] HDMI cable: Pi → Display
- [ ] 5V power to display

### Power
- [ ] 5V 2A USB power to Pi
- [ ] All GND connections tied together

## 9. Component Shopping List

### Essential
- 1x Raspberry Pi Zero 2W ($15)
- 1x MicroSD Card 8GB+ ($5)
- 1x 480x480 Round HDMI Display ($25-40)
- 1x ADS1115 ADC Module ($5)
- 4x 10kΩ Linear Potentiometers ($2 each)
- 3x SPST Toggle Switches ($1 each)
- 1x TRS 3.5mm Jack (Female) ($2)
- 2x 2N2222 Transistors ($0.20 each)
- 4x 220Ω Resistors ($0.05 each)
- 4x 1kΩ Resistors ($0.05 each)
- Breadboard/PCB ($5-15)
- Jumper wires ($5)

### Optional
- 2x 3.5mm Mono Jacks for triggers ($2 each)
- Eurorack panel material (12HP) ($10-20)
- Knob caps ($2 each)
- 3D printed bezel ($5 filament)

**Total Cost:** ~$100-130 for complete build

## 10. Testing Checklist

After wiring, test each system:

```bash
# Run hardware test script
sudo ./test-hardware.sh
```

Or manually test:

```bash
# Test I2C
sudo i2cdetect -y 1  # Should show 0x48

# Test GPIO input
gpio -g read 17      # Read switch state

# Test GPIO output  
gpio -g write 23 1   # Turn on trigger
gpio -g write 23 0   # Turn off trigger

# Test UART
echo "test" > /dev/serial0  # Send test byte
```

═══════════════════════════════════════════════════════════════════════════════

For circuit board design files and KiCad schematics:
https://github.com/DrFunn1/dryer-eurorack/tree/main/hardware

═══════════════════════════════════════════════════════════════════════════════
