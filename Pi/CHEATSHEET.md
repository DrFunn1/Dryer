# Dryer Eurorack - Quick Reference Cheat Sheet

## Initial Setup (One Time)

### 1. Create GitHub Repo & Push
```powershell
# Windows PowerShell
cd "H:\Shared drives\GitHub\Dryer-pi"

# Initialize git
git init
git add .
git commit -m "Initial commit"

# CREATE REPO ON GITHUB FIRST:
# Go to https://github.com/new
# Name: dryer-pi
# Public/Private: your choice
# DO NOT initialize with README
# Click "Create repository"
# Copy the repo URL shown

# Now push:
git remote add origin https://github.com/YOUR_USERNAME/dryer-pi.git
git branch -M main
git push -u origin main
# (Use Personal Access Token as password)
```

### 2. Flash SD Card
- Use Raspberry Pi Imager
- OS: Raspberry Pi OS Lite (64-bit)
- Configure: SSH, username=`dryer`, hostname=`dryer-pi`, WiFi

### 3. First Boot
```bash
ssh dryer@dryer-pi.local
sudo apt update && sudo apt install -y git
cd ~
git clone https://github.com/YOUR_USERNAME/dryer-pi.git
cd dryer-pi
sudo ./setup.sh
sudo reboot
```

---

## Daily Development

### Edit → Push → Deploy

```powershell
# On Windows:
cd "H:\Shared drives\GitHub\Dryer-pi"
# [edit files]
git add .
git commit -m "Description of changes"
git push
```

```bash
# On Pi:
ssh dryer@dryer-pi.local
cd ~/dryer-pi
git pull
make clean && make && sudo make install
sudo systemctl restart dryer.service
```

---

## Quick Commands

```bash
# SSH in
ssh dryer@dryer-pi.local

# Update & rebuild (one line)
cd ~/dryer-pi && git pull && make clean && make && sudo make install && sudo systemctl restart dryer.service

# Watch logs
sudo journalctl -u dryer.service -f

# Manual run (for debugging)
sudo systemctl stop dryer.service
cd ~/dryer-pi
sudo ./dryer

# Check status
sudo systemctl status dryer.service

# Test hardware
cd ~/dryer-pi
sudo ./test-hardware.sh

# Check I2C
sudo i2cdetect -y 1

# Reboot
sudo reboot
```

---

## File Locations

```
~/dryer-pi/                           # Source code (git repo)
/usr/local/bin/dryer                  # Installed binary
/etc/systemd/system/dryer.service     # Auto-start service
/boot/config.txt                      # Display config
```

---

## Pin Assignments

```
ADC (ADS1115 on I2C):
  A0 → RPM pot (1-40)
  A1 → Drum Size pot (60-100 cm)
  A2 → Vanes pot (1-9)
  A3 → Vane Height pot (10-50%)

GPIO Inputs:
  17 → Ball Type switch
  27 → Lint Trap switch
  22 → Moon Gravity switch

GPIO Outputs:
  23 → Trigger 1 (drum collisions)
  24 → Trigger 2 (vane collisions)
  14 → MIDI TX (UART)
```

---

## Troubleshooting

```bash
# Can't SSH?
ping dryer-pi.local        # Find IP, use that instead

# Build fails?
sudo ./setup.sh            # Re-run setup

# Display not working?
cat /boot/config.txt | grep hdmi
# Should show: hdmi_cvt=480 480 60 1 0 0 0

# ADC not detected?
sudo i2cdetect -y 1        # Should show 0x48
# If not: sudo raspi-config → I2C → Enable

# Service won't start?
sudo journalctl -u dryer.service -n 50
# Check logs for errors
```

---

## System Control

```bash
# Service management
sudo systemctl start dryer.service
sudo systemctl stop dryer.service
sudo systemctl restart dryer.service
sudo systemctl status dryer.service

# Enable/disable auto-start
sudo systemctl enable dryer.service
sudo systemctl disable dryer.service

# View logs
sudo journalctl -u dryer.service -f      # Follow in real-time
sudo journalctl -u dryer.service -n 50   # Last 50 lines
sudo journalctl -u dryer.service -b      # Since last boot
```

---

## Git Shortcuts

```bash
# Status
git status
git log --oneline -10

# Discard local changes
git reset --hard origin/main

# Create branch
git checkout -b feature/new-feature
git push -u origin feature/new-feature

# Switch branches
git checkout main
git checkout feature/new-feature

# Delete branch
git branch -d feature/new-feature
git push origin --delete feature/new-feature
```

---

## Performance Info

- Boot time: ~10-15 seconds
- Physics rate: 240 Hz
- Display: 60 FPS
- CPU usage: 30-40%
- ADC polling: 20 Hz

---

## Support

- Full docs: README.md
- Git workflow: GIT_WORKFLOW.md
- Hardware wiring: WIRING.md
- Quick start: QUICKSTART.md

---

## One-Liner Deploy Script

Save as `deploy.sh`:
```bash
#!/bin/bash
cd ~/dryer-pi && git pull && make clean && make && sudo make install && sudo systemctl restart dryer.service && echo "✓ Deployed" && sudo journalctl -u dryer.service -n 5
```

Usage: `chmod +x deploy.sh && ./deploy.sh`
