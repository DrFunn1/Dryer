# Dryer Eurorack Module - Quick Start Guide

## For Impatient Hardware Hackers

### What You Need (Minimum)
- Raspberry Pi Zero 2W with SD card (8GB+)
- 480x480 HDMI display
- ADS1115 ADC module
- 4x 10kΩ pots, 3x toggle switches
- Breadboard and jumper wires (for testing)

### 30-Minute Setup

#### 1. Flash SD Card (5 min)
```bash
# Use Raspberry Pi Imager
# Select: Raspberry Pi OS Lite (64-bit)
# Configure WiFi and SSH in advanced options
# Flash to SD card
```

#### 2. First Boot (2 min)
```bash
# Insert SD card, connect HDMI and power
# Wait for boot (~30 seconds)
# SSH in: ssh pi@raspberrypi.local
```

#### 3. Wire Hardware (10 min)
```
ADS1115 to Pi:
  VDD → Pin 1 (3.3V)
  GND → Pin 6 (GND)
  SCL → Pin 5 (GPIO 3)
  SDA → Pin 3 (GPIO 2)

Potentiometers:
  Each pot: GND - Wiper - 3.3V
  Wipers → ADS1115 A0, A1, A2, A3

Switches (optional for testing):
  GPIO 17, 27, 22 → 3.3V via switch

Display:
  Connect via HDMI
```

#### 4. Run Setup Script (10 min)
```bash
# Copy files to Pi
scp -r dryer-cpp pi@raspberrypi.local:~/dryer-eurorack

# SSH in
ssh pi@raspberrypi.local

# Run setup
cd ~/dryer-eurorack
sudo ./setup.sh
```

Setup script will:
- Install all dependencies
- Enable I2C and UART
- Build the code
- Install systemd service
- Optimize boot time

#### 5. Test Hardware (2 min)
```bash
sudo ./test-hardware.sh
```

This verifies all connections are working.

#### 6. Reboot and Run (1 min)
```bash
sudo reboot
```

After reboot, Dryer starts automatically!

Watch it start:
```bash
sudo journalctl -u dryer.service -f
```

---

## Troubleshooting

### "Can't detect ADS1115"
```bash
sudo i2cdetect -y 1
# Should show 0x48
# If not, check wiring
```

### "Display not working"
```bash
# Edit HDMI config
sudo nano /boot/config.txt

# Add these lines:
hdmi_group=2
hdmi_mode=87
hdmi_cvt=480 480 60 1 0 0 0
hdmi_force_hotplug=1

# Reboot
sudo reboot
```

### "MIDI not working"
```bash
# Check UART is enabled
ls -l /dev/serial0

# Should exist and point to ttyAMA0
# If not, run raspi-config:
sudo raspi-config
# Interface Options → Serial Port
# Login shell: NO
# Serial hardware: YES
```

### "Permission denied" errors
```bash
# Always run with sudo:
sudo ./dryer

# Or run as service (runs as root):
sudo systemctl start dryer.service
```

---

## Parameter Tuning

### Physical Parameters (match your real-world dimensions)
- **RPM:** 1-40 (start with 20)
- **Drum Size:** 60-100 cm (your actual drum diameter)
- **Vanes:** 1-9 (interior baffles)
- **Vane Height:** 10-50% (how far vanes extend)

### Fun Presets to Try

**Slow Tumble (gentle)**
- RPM: 5
- Vanes: 3
- Vane Height: 20%

**Fast Chaos (aggressive)**
- RPM: 35
- Vanes: 7
- Vane Height: 45%

**Minimal (simple patterns)**
- RPM: 15
- Vanes: 1
- Vane Height: 30%

---

## What's Happening?

The physics engine simulates a tennis ball bouncing in a rotating drum with internal vanes (like a real clothes dryer). Each collision generates:

1. **MIDI Note** - Different note for each surface
2. **Audio Click** - Visual feedback on display
3. **CV Trigger** - 10ms pulse for eurorack gear

The patterns are **deterministic but chaotic** - same parameters = same rhythm, but small changes create completely different patterns.

---

## Next Steps

### Add Physical Panel
- Drill 12HP eurorack panel (60.6mm wide)
- Mount 2.1" round display
- Add knobs and switches
- 3D print or laser cut bezel

### Add Audio Output
- Install ALSA for real audio synthesis
- Or keep it MIDI-only and use external synth

### Fine-tune Physics
- Edit `dryer-physics.cpp`
- Adjust restitution, drag, gravity
- Try different ball types

### Create Variations
- Multiple ball types
- Variable gravity
- Magnetic forces?

---

## Daily Operation

### Start/Stop
```bash
sudo systemctl start dryer.service   # Start
sudo systemctl stop dryer.service    # Stop
sudo systemctl restart dryer.service # Restart
```

### View Logs
```bash
# Real-time log
sudo journalctl -u dryer.service -f

# Last 50 lines
sudo journalctl -u dryer.service -n 50
```

### Disable Auto-start
```bash
sudo systemctl disable dryer.service
```

### Manual Run (for debugging)
```bash
sudo systemctl stop dryer.service  # Stop service first
sudo ./dryer                       # Run manually
```

---

## Performance Stats

**Typical Performance (Pi Zero 2W):**
- Boot time: 8-15 seconds
- CPU usage: 30-40%
- Frame rate: 60 FPS (VSync)
- Physics rate: 240 Hz (60×4 substeps)
- ADC polling: 20 Hz

**Expected Timing:**
- Power on → Display active: ~12 seconds
- Total boot → running: ~15 seconds
- Physics latency: <4ms
- MIDI latency: <2ms

---

## Safety Notes

⚠️ **Important:**
- Pi Zero 2W gets warm under load - ensure adequate cooling
- SD card can wear out - consider read-only filesystem for 24/7 use
- MIDI voltage is 5V - don't connect directly to 3.3V devices
- Eurorack triggers use external 5V - verify your transistor circuit

---

For detailed documentation, see `README.md`

Hardware schematics and PCB files: Coming soon™
