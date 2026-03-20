# Dryer Eurorack Module - C++ Port

Hardware eurorack percussion generator for Raspberry Pi Zero 2W. Direct port of the JavaScript physics simulation with hardware I/O integration.

## Hardware Requirements

### Core Components
- **Raspberry Pi Zero 2W** (or Pi 4/5 for development)
- **480x480 Round HDMI Display** (Waveshare or similar)
- **ADS1115 16-bit ADC** (I2C, 4 channels for potentiometers)
- **4x 10kΩ Linear Potentiometers** (0-3.3V)
- **3x Toggle Switches** (SPST, 3.3V logic)
- **2x 2N2222 Transistors** (for 0-3.3V → 0-5V trigger conversion)
- **MIDI TRS Jack** (Type A or B, depending on your gear)

### Pin Assignments

See `pins.h` for complete GPIO mapping:

**ADC Channels (ADS1115 on I2C bus 1):**
- A0: RPM (1-40)
- A1: Drum Size (60-100 cm)
- A2: Vanes (1-9)
- A3: Vane Height (10-50%)

**GPIO Inputs (3.3V switches):**
- GPIO 17: Ball Type (tennis/balloon)
- GPIO 27: Lint Trap Enable
- GPIO 22: Moon Gravity Enable

**GPIO Outputs:**
- GPIO 23: Trigger Out 1 (drum collisions)
- GPIO 24: Trigger Out 2 (vane collisions)
- UART TX (GPIO 14): MIDI output

## Software Setup

### 1. Install Raspberry Pi OS Lite

Use Raspberry Pi Imager to install **Raspberry Pi OS Lite (64-bit)**. This gives you a minimal system for fast boot times.

### 2. Enable Required Interfaces

```bash
sudo raspi-config
```

Enable:
- **I2C** (Interface Options → I2C → Enable)
- **Serial Port** (Interface Options → Serial → Login shell: NO, Serial hardware: YES)

Reboot after changes:
```bash
sudo reboot
```

### 3. Install Dependencies

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install build tools
sudo apt install -y build-essential cmake git

# Install SDL2
sudo apt install -y libsdl2-dev

# Install libgpiod (GPIO library)
sudo apt install -y libgpiod-dev gpiod

# Install I2C tools (for testing)
sudo apt install -y i2c-tools
```

### 4. Verify Hardware Connections

Test I2C (should see ADS1115 at address 0x48):
```bash
sudo i2cdetect -y 1
```

Test GPIO:
```bash
# Read a switch
gpio -g read 17

# Test trigger output
gpio -g mode 23 out
gpio -g write 23 1
sleep 0.1
gpio -g write 23 0
```

## Building the Project

### Clone or Copy Source Files

```bash
mkdir -p ~/dryer-eurorack
cd ~/dryer-eurorack

# Copy all .cpp, .h files and CMakeLists.txt here
```

### Build with CMake

```bash
mkdir build
cd build
cmake ..
make -j4

# Install to system (optional)
sudo make install
```

### Build Output

Executable: `build/dryer`

## Running

### Manual Run

```bash
cd ~/dryer-eurorack/build
sudo ./dryer
```

**Note:** Must run with `sudo` for GPIO access.

### Auto-Start on Boot

Create systemd service:

```bash
sudo nano /etc/systemd/system/dryer.service
```

Add:
```ini
[Unit]
Description=Dryer Eurorack Percussion Generator
After=network.target pigpiod.service

[Service]
Type=simple
User=root
WorkingDirectory=/home/pi/dryer-eurorack/build
ExecStart=/home/pi/dryer-eurorack/build/dryer
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable dryer.service
sudo systemctl start dryer.service

# Check status
sudo systemctl status dryer.service

# View logs
sudo journalctl -u dryer.service -f
```

## Hardware Wiring

### ADS1115 ADC Connection

```
ADS1115          Raspberry Pi
VDD       -----> 3.3V (Pin 1)
GND       -----> GND (Pin 6)
SCL       -----> GPIO 3 (SCL, Pin 5)
SDA       -----> GPIO 2 (SDA, Pin 3)
A0-A3     -----> Potentiometer wipers (0-3.3V)
```

### Potentiometer Wiring

Each pot:
```
Pin 1 (CCW) -----> GND
Pin 2 (Wiper) ---> ADS1115 A0-A3
Pin 3 (CW) ------> 3.3V
```

### Toggle Switches

```
Switch           Raspberry Pi
Common    -----> 3.3V
NO/NC     -----> GPIO (17, 27, 22)
```

Internal pull-down resistors enabled in software.

### MIDI Output Circuit

```
GPIO 14 (UART TX) ---[220Ω]--- TRS Tip (Pin 4)
                   |
                   +--[220Ω]--- TRS Ring (Pin 5)
                   
TRS Sleeve (Pin 2) ------------ GND
```

For MIDI Type A: Tip=4, Ring=5, Sleeve=2
For MIDI Type B: Swap Tip and Ring

### Trigger Outputs (Eurorack CV)

```
                      +5V (from eurorack power)
                       |
                     [1kΩ]
                       |
GPIO 23/24 ---[1kΩ]---[2N2222]--- Trigger Out Jack Tip
                     (E)  (C)
                      |
                     GND
```

This converts 3.3V GPIO to 0-5V eurorack standard.

## Usage

### Physical Controls

**Knobs:**
- RPM: Drum rotation speed (1-40 RPM)
- Drum Size: Physical diameter (60-100 cm)
- Vanes: Interior baffles (1-9)
- Vane Height: How far vanes extend (10-50%)

**Switches:**
- Ball Type: Tennis ball (default) or Balloon (large, light)
- Lint Trap: Filter out low-velocity collisions
- Moon Gravity: 1/6th Earth gravity mode

### MIDI Output

- Each collision surface generates a unique MIDI note
- Base note: C2 (MIDI 36)
- Notes assigned chromatically as surfaces are created
- Velocity scales with collision impact (0-127)
- 100ms note duration

### CV Trigger Outputs

- **Trigger 1:** Fires on drum wall collisions
- **Trigger 2:** Fires on vane collisions
- 10ms pulse width (configurable in pins.h)
- 0-5V eurorack standard

## Performance Optimization

### Boot Time Optimization

Current ~15-20 seconds from power-on to running. To improve:

**1. Reduce boot services:**
```bash
sudo systemctl disable bluetooth.service
sudo systemctl disable avahi-daemon.service
sudo systemctl disable triggerhappy.service
```

**2. Use read-only filesystem** (optional, for reliability):
```bash
sudo raspi-config
# Performance Options → Overlay File System → Enable
```

**3. Minimize kernel modules:**
Edit `/boot/config.txt`:
```
# Disable unused hardware
dtoverlay=disable-bt
dtoverlay=disable-wifi
```

Target: **8-12 second boot time**

### Runtime Performance

- Physics loop: 240Hz (60 FPS × 4 substeps)
- ADC polling: 20Hz (50ms interval)
- Display: 60 FPS (VSync)
- CPU usage: ~30-40% on Pi Zero 2W

## Troubleshooting

### Display Issues

**Black screen:**
- Check HDMI connection
- Verify `/boot/config.txt` has correct HDMI settings
- Try `hdmi_force_hotplug=1`

**Wrong resolution:**
Edit `/boot/config.txt`:
```
hdmi_group=2
hdmi_mode=87
hdmi_cvt=480 480 60 1 0 0 0
```

### ADC Not Detected

```bash
# Check I2C is enabled
sudo i2cdetect -y 1

# Should show 0x48 (ADS1115 default address)
# If not, check wiring and power
```

### MIDI Not Working

```bash
# Check UART is enabled
ls -l /dev/serial0

# Test UART transmission
echo "test" > /dev/serial0

# For true 31.25kbaud, may need custom clock:
sudo nano /boot/config.txt
# Add: init_uart_clock=2441406
```

### GPIO Permission Errors

```bash
# Make sure pigpiod is running
sudo systemctl status pigpiod

# Run dryer with sudo
sudo ./dryer
```

## Development

### Debug Mode

Compile with debug symbols:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
gdb ./dryer
```

### Code Structure

```
pins.h              - Hardware pin definitions
dryer-physics.*     - Physics simulation engine (pure math)
dryer-hardware.*    - I2C, GPIO, MIDI I/O
dryer-renderer.*    - SDL2 graphics for display
dryer-main.cpp      - Main application controller
```

### Adding Features

**New ball types:**
Edit `dryer-physics.cpp`, add preset in `setBallProperties()`.

**New parameter ranges:**
Edit `pins.h`, modify `ParamRanges` struct.

**Custom MIDI mapping:**
Edit `dryer-main.cpp`, modify `assignMIDINotes()`.

## License

MIT License - Free for commercial eurorack products

## Credits

Original JavaScript version: Edward (DrFunn.com)
C++ Port: Edward
Physics Engine: Based on rotating reference frame simulation with Coriolis and centrifugal forces

---

**Hardware Module Status:** Beta - Firmware tested, PCB in progress

For support and updates: https://github.com/DrFunn1/dryer-eurorack
