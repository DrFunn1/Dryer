# Development Workflow Guide

## Complete Setup Process (Step-by-Step)

### Phase 1: SD Card Preparation (5 minutes)

1. **Download Raspberry Pi Imager**
   - Windows/Mac/Linux: https://www.raspberrypi.com/software/

2. **Configure & Flash**
   ```
   [Raspberry Pi Imager]
   
   Device: Raspberry Pi Zero 2 W
   OS: Raspberry Pi OS Lite (64-bit) ← IMPORTANT: Choose "Lite"
   Storage: [Your SD Card]
   
   Click ⚙️ (Settings):
   ✅ Set hostname: dryerpi
   ✅ Enable SSH (use password authentication)
   ✅ Set username: pi
   ✅ Set password: [your password]
   ✅ Configure wireless LAN:
      SSID: [your WiFi name]
      Password: [your WiFi password]
      Country: US (or your country)
   ✅ Set locale settings
      Timezone: [your timezone]
      Keyboard: us
   
   Click SAVE, then WRITE
   ```

3. **First Boot**
   - Insert SD card in Pi
   - Connect power (nothing else needed yet!)
   - Wait ~60 seconds for first boot
   - Pi will connect to WiFi automatically

4. **Test SSH Connection**
   ```bash
   # From your laptop/desktop:
   ssh pi@dryerpi.local
   # Enter password when prompted
   
   # If .local doesn't work, find IP:
   # Check your router, or use: ping dryerpi.local
   ```

**You're now ready to develop entirely via SSH - no monitor needed!**

---

### Phase 2: Development Setup (SSH method - RECOMMENDED)

**Why SSH-only is better:**
- Work from your comfortable dev machine
- Copy/paste commands easily
- No monitor switching needed
- Test on regular monitor only when ready

**Workflow:**

```bash
# === ON YOUR DEV MACHINE ===

# 1. Extract the project
tar -xzf dryer-cpp-complete.tar.gz
cd dryer-cpp

# 2. Copy to Pi via SCP
scp -r ../dryer-cpp pi@dryerpi.local:~/

# === NOW SSH INTO PI ===
ssh pi@dryerpi.local

# 3. Run setup (this installs everything)
cd dryer-cpp
sudo ./setup.sh

# Setup will:
# - Install build tools (gcc, make, SDL2, pigpio)
# - Enable I2C and UART
# - Build the project
# - Install systemd service
# - Configure for round display
# Wait 5-10 minutes...

# 4. Test hardware connections
sudo ./test-hardware.sh

# 5. If using regular monitor for testing, toggle display:
sudo ./toggle-display.sh  # Switch to regular monitor
sudo reboot

# Connect regular HDMI monitor, then:
sudo systemctl start dryer.service
sudo journalctl -u dryer.service -f  # Watch logs

# 6. When ready for round display:
sudo systemctl stop dryer.service
sudo ./toggle-display.sh  # Switch back to round display
sudo reboot

# Now runs on round display automatically!
```

---

### Phase 3: Development Cycle

**Editing Code:**

```bash
# Option A: Edit on Pi via SSH (use nano/vim)
ssh pi@dryerpi.local
cd dryer-cpp
nano dryer-physics.cpp

# Option B: Edit on dev machine, then copy over
# On dev machine:
nano dryer-cpp/dryer-physics.cpp
scp dryer-cpp/dryer-physics.cpp pi@dryerpi.local:~/dryer-cpp/

# Then rebuild on Pi:
ssh pi@dryerpi.local
cd dryer-cpp
make clean && make
sudo systemctl restart dryer.service
```

**Testing Changes:**

```bash
# Stop auto-running service
sudo systemctl stop dryer.service

# Run manually to see debug output
sudo ./dryer

# Press Ctrl+C to stop

# When satisfied, install and restart service:
sudo make install
sudo systemctl start dryer.service
```

---

## Make vs CMake - Which to Use?

**TL;DR: Use `make` - it's simpler.**

### Makefile (Recommended)
```bash
make              # Build
make clean        # Clean
make install      # Install to /usr/local/bin
make run          # Build and run with sudo
```

**Pros:**
- Simpler
- No extra dependencies
- Everyone knows Make
- Faster for small projects

### CMake (Alternative)
```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

**Pros:**
- Better for large projects
- More portable (Windows/Mac/Linux)
- Better IDE integration

**For this project:** Makefile is simpler. The setup.sh script uses Make by default.

---

## Git - Do You Need It?

### Short Answer: **No, not required**

**Setup script handles everything:**
- No git clone needed
- You copy files via SCP
- Project is self-contained

### But Git is Useful For:

**Version Control:**
```bash
cd ~/dryer-cpp
git init
git add .
git commit -m "Initial working version"

# Make changes...
git commit -am "Adjusted physics parameters"

# Revert if needed
git checkout -- dryer-physics.cpp
```

**Keeping Updated:**
```bash
# If you push to GitHub later:
git pull origin main  # Get updates
make clean && make    # Rebuild
```

**Install git (optional):**
```bash
sudo apt install git
```

**Verdict:** Install git if you like version control, skip it if you just want to test hardware.

---

## Monitor Switching - Detailed Guide

### Method 1: Manual Config Edit

**For Development (Regular Monitor):**
```bash
sudo nano /boot/config.txt

# Find and comment out (add #):
#hdmi_group=2
#hdmi_mode=87
#hdmi_cvt=480 480 60 1 0 0 0

# Add if not present:
hdmi_force_hotplug=1

# Save and reboot:
sudo reboot
```

**For Production (Round Display):**
```bash
sudo nano /boot/config.txt

# Uncomment (remove #):
hdmi_group=2
hdmi_mode=87
hdmi_cvt=480 480 60 1 0 0 0
hdmi_force_hotplug=1

sudo reboot
```

### Method 2: Toggle Script (Easier)

```bash
# Included in the project:
sudo ./toggle-display.sh

# It will ask if you want to reboot
# Say yes, wait 30 seconds
```

### Method 3: Two Config Files (Advanced)

```bash
# Create two configs:
sudo cp /boot/config.txt /boot/config-round.txt
sudo cp /boot/config.txt /boot/config-regular.txt

# Edit each one, then swap:
sudo cp /boot/config-round.txt /boot/config.txt
sudo reboot

# Or:
sudo cp /boot/config-regular.txt /boot/config.txt
sudo reboot
```

---

## Recommended Setup Sequence

### Week 1: Breadboard Prototype (SSH-only)
```bash
# Use SSH for everything
# No monitor connected
# Test hardware with test-hardware.sh
# Wire pots and switches on breadboard
# Verify ADC readings
# Test MIDI output with DAW
```

### Week 2: Basic Testing (Regular Monitor)
```bash
# Connect regular HDMI monitor
# Use toggle-display.sh to switch to regular monitor
# Run dryer manually: sudo ./dryer
# Verify graphics rendering
# Test all knobs and switches
# Debug any issues
```

### Week 3: Final Hardware (Round Display)
```bash
# Wire up round display
# Use toggle-display.sh to switch to round display
# Enable auto-start: sudo systemctl enable dryer.service
# Test boot-to-running time
# Verify all features work
```

### Week 4: Integration
```bash
# Mount in eurorack case
# Connect MIDI to synth
# Connect triggers to modules
# Final testing
```

---

## Common Development Tasks

### Viewing Logs
```bash
# Real-time logs
sudo journalctl -u dryer.service -f

# Last 50 lines
sudo journalctl -u dryer.service -n 50

# All logs since boot
sudo journalctl -u dryer.service -b
```

### Checking Status
```bash
# Service status
sudo systemctl status dryer.service

# Is pigpio running?
sudo systemctl status pigpiod

# CPU/memory usage
htop

# I2C devices
sudo i2cdetect -y 1
```

### Quick Rebuild
```bash
cd ~/dryer-cpp
make clean && make && sudo make install
sudo systemctl restart dryer.service
```

### Reset Everything
```bash
# Nuclear option - start fresh
cd ~
rm -rf dryer-cpp
# Re-copy files and run setup.sh
```

---

## Troubleshooting

### "Cannot connect via SSH"
```bash
# Try IP address instead of .local
# Find IP on router, or:
ping dryerpi.local  # Note the IP

ssh pi@192.168.1.XXX
```

### "Build fails - missing library"
```bash
# Re-run setup script
cd ~/dryer-cpp
sudo ./setup.sh
```

### "Display shows nothing"
```bash
# Check HDMI is connected
# Try forcing HDMI:
sudo nano /boot/config.txt
# Add: hdmi_force_hotplug=1
sudo reboot
```

### "ADC not detected"
```bash
# Check I2C is enabled
sudo i2cdetect -y 1
# Should show 0x48

# If not, enable I2C:
sudo raspi-config
# Interface Options → I2C → Enable
sudo reboot
```

---

## Tools You'll Want

### Installed by setup.sh:
- ✅ gcc/g++ (compiler)
- ✅ make (build system)
- ✅ SDL2 (graphics)
- ✅ pigpio (GPIO library)
- ✅ i2c-tools (I2C debugging)

### Useful extras:
```bash
# Text editor with syntax highlighting
sudo apt install vim

# System monitor
sudo apt install htop

# Network tools
sudo apt install net-tools

# Git (optional)
sudo apt install git
```

---

## Pro Tips

1. **Use tmux for persistent SSH sessions:**
   ```bash
   sudo apt install tmux
   tmux new -s dryer
   # Work...
   # Detach: Ctrl+B, then D
   # Reattach: tmux attach -t dryer
   ```

2. **Backup SD card image:**
   ```bash
   # On Linux/Mac:
   sudo dd if=/dev/sdX of=dryer-backup.img bs=4M status=progress
   # Replace /dev/sdX with your SD card device
   ```

3. **Read-only filesystem for reliability:**
   ```bash
   # After everything works:
   sudo raspi-config
   # Performance → Overlay FS → Enable
   # Prevents SD card corruption on power loss
   ```

4. **Auto-connect to serial console (for debugging):**
   ```bash
   # USB serial adapter on UART pins
   screen /dev/ttyUSB0 115200
   ```

---

## My Recommended Workflow

```bash
# === INITIAL SETUP (ONE TIME) ===
# 1. Flash SD card with Raspberry Pi Imager (WiFi + SSH configured)
# 2. Boot Pi, wait 60 seconds
# 3. SSH in: ssh pi@dryerpi.local
# 4. Copy files: scp -r dryer-cpp pi@dryerpi.local:~/
# 5. Run setup: cd dryer-cpp && sudo ./setup.sh
# 6. Test hardware: sudo ./test-hardware.sh

# === DAILY DEVELOPMENT ===
# - Edit code on dev machine
# - SCP changed files to Pi
# - SSH in and run: make clean && make && sudo ./dryer
# - Test, iterate, repeat

# === WHEN READY FOR ROUND DISPLAY ===
# - sudo ./toggle-display.sh
# - sudo reboot
# - Connect round display
# - Done!
```

This workflow keeps you on your comfortable dev machine for editing, while the Pi runs headless until you're ready to test graphics.
