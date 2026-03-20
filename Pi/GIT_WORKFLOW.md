# Git Workflow for Dryer Eurorack Module

## Complete Setup with GitHub

### Prerequisites
- GitHub account
- Local repo at: `H:\Shared drives\GitHub\Dryer-pi`
- Raspberry Pi Zero 2W with fresh SD card

---

## Part 1: Push to GitHub (Windows PowerShell)

### 1.1 Initialize Git Repo (if not already done)

```powershell
cd "H:\Shared drives\GitHub\Dryer-pi"

# Initialize git
git init

# Add all files
git add .

# First commit
git commit -m "Initial commit - Dryer eurorack module"
```

### 1.2 Create GitHub Repository FIRST

**You MUST create the repo on GitHub before pushing!**

**Option A: Via GitHub Website (Easiest)**
1. Go to https://github.com/new
2. Repository name: `dryer-pi`
3. Description: "Eurorack percussion generator - physics simulation on Raspberry Pi"
4. **Public** or **Private** (your choice)
5. ❌ **Do NOT** initialize with README, .gitignore, or license (you already have files)
6. Click "Create repository"
7. **Copy the repository URL** shown (e.g., `https://github.com/YOUR_USERNAME/dryer-pi.git`)

**Option B: Via GitHub CLI (Faster if you have it)**
```powershell
# Install GitHub CLI if needed:
# https://cli.github.com/

# Create repo and push in one command:
gh repo create dryer-pi --public --source=. --remote=origin --push

# That's it! Skip to Part 2 if using this method.
```

### 1.3 Push to GitHub

```powershell
# Add remote (use the URL you copied from GitHub)
git remote add origin https://github.com/YOUR_USERNAME/dryer-pi.git

# Rename branch to main
git branch -M main

# Push to GitHub
git push -u origin main

# Enter your credentials:
# Username: YOUR_USERNAME
# Password: [Use Personal Access Token, not password!]
#   Create token at: https://github.com/settings/tokens
#   Click "Generate new token (classic)"
#   Select scope: repo (all checkboxes)
#   Copy token and use as password
```

**If you need to authenticate:**
- Use Personal Access Token (PAT) instead of password
- Create PAT at: https://github.com/settings/tokens
- Or use GitHub Desktop for easier auth

---

## Part 2: SD Card Preparation

### 2.1 Flash with Raspberry Pi Imager

```
[Raspberry Pi Imager]

Choose Device: Raspberry Pi Zero 2 W
Choose OS: Raspberry Pi OS Lite (64-bit)  ← 64-bit is important!
Choose Storage: [Your SD Card]

Click ⚙️ (Advanced Options):
┌────────────────────────────────────────┐
│ ✅ Set hostname: dryer-pi              │
│ ✅ Enable SSH                          │
│    ◉ Use password authentication       │
│ ✅ Set username and password:          │
│    Username: dryer                     │
│    Password: [your secure password]    │
│ ✅ Configure wireless LAN:             │
│    SSID: [your WiFi name]              │
│    Password: [your WiFi password]      │
│    Country: US (or your location)      │
│ ✅ Set locale settings:                │
│    Timezone: America/New_York          │
│    Keyboard layout: us                 │
└────────────────────────────────────────┘

Click SAVE → WRITE → Wait for completion
```

### 2.2 First Boot

- Insert SD card into Pi
- Connect round display via HDMI
- Connect power
- Wait ~60 seconds for first boot
- Pi will connect to WiFi automatically

---

## Part 3: SSH Setup & Git Clone

### 3.1 SSH Connection

```powershell
# From Windows PowerShell:
ssh dryer@dryer-pi.local

# If .local doesn't work, find IP address:
# Check your router, or use Advanced IP Scanner
# Then: ssh dryer@192.168.1.XXX
```

### 3.2 Install Git

```bash
# Once SSH'd into the Pi:
sudo apt update
sudo apt install -y git vim

# Configure git (optional but good practice):
git config --global user.name "Your Name"
git config --global user.email "your@email.com"
```

### 3.3 Clone Repository

```bash
# Clone into home directory
cd ~
git clone https://github.com/YOUR_USERNAME/dryer-pi.git

# Enter your credentials when prompted
# (Use Personal Access Token as password)

# Navigate into repo
cd dryer-pi
ls -la  # Verify all files are present
```

---

## Part 4: Installation

### 4.1 Run Setup Script

```bash
cd ~/dryer-pi
sudo ./setup.sh
```

**This will:**
- Install all dependencies (SDL2, pigpio, i2c-tools)
- Enable I2C and UART interfaces
- Configure 480x480 round display (ONLY config)
- Disable Bluetooth and WiFi for boot optimization
- Build the project
- Install to /usr/local/bin/dryer
- Create systemd service for auto-start
- Optimize boot time (~10-15 seconds)

**Wait 5-10 minutes for completion.**

### 4.2 Test Hardware

```bash
cd ~/dryer-pi
sudo ./test-hardware.sh
```

This verifies:
- ✓ I2C bus (ADS1115 at 0x48)
- ✓ UART device (/dev/serial0)
- ✓ GPIO inputs (switches)
- ✓ GPIO outputs (triggers)
- ✓ ADC channel readings

### 4.3 Reboot & Run

```bash
sudo reboot
```

After reboot:
- Dryer starts automatically
- Display shows animation
- Knobs control parameters in real-time
- MIDI and triggers output on collisions

---

## Part 5: Development Workflow

### 5.1 Make Changes on Windows

```powershell
# Edit files in your local repo
cd "H:\Shared drives\GitHub\Dryer-pi"
code dryer-physics.cpp  # Or your editor of choice

# Commit changes
git add dryer-physics.cpp
git commit -m "Adjusted ball restitution coefficient"

# Push to GitHub
git push
```

### 5.2 Pull on Pi & Rebuild

```bash
# SSH into Pi
ssh dryer@dryer-pi.local

# Pull latest changes
cd ~/dryer-pi
git pull

# Rebuild
make clean && make
sudo make install

# Restart service
sudo systemctl restart dryer.service
```

### 5.3 Watch Logs

```bash
# Real-time log viewing
sudo journalctl -u dryer.service -f

# Last 50 lines
sudo journalctl -u dryer.service -n 50

# Since last boot
sudo journalctl -u dryer.service -b
```

---

## Part 6: Common Tasks

### View Status

```bash
# Service status
sudo systemctl status dryer.service

# Is it running?
pgrep dryer

# CPU usage
htop
```

### Stop/Start Service

```bash
# Stop auto-running service
sudo systemctl stop dryer.service

# Run manually for debugging
cd ~/dryer-pi
sudo ./dryer

# Press Ctrl+C to stop

# Start service again
sudo systemctl start dryer.service
```

### Update from GitHub

```bash
cd ~/dryer-pi
git pull
make clean && make && sudo make install
sudo systemctl restart dryer.service
```

### Check Hardware

```bash
# I2C devices
sudo i2cdetect -y 1

# Read ADC values
cd ~/dryer-pi
sudo ./test-hardware.sh

# GPIO state
gpio -g read 17  # Ball type switch
gpio -g read 27  # Lint trap switch
gpio -g read 22  # Moon gravity switch
```

---

## Part 7: GitHub Best Practices

### Branching Strategy

```bash
# Create feature branch
git checkout -b feature/new-ball-type

# Make changes...
git add .
git commit -m "Add baseball ball type"

# Push feature branch
git push -u origin feature/new-ball-type

# Merge when ready (via GitHub PR or locally):
git checkout main
git merge feature/new-ball-type
git push
```

### Tagging Releases

```bash
# Tag working versions
git tag -a v1.0 -m "First working version"
git push --tags

# Later, checkout a specific version:
git checkout v1.0
```

### .gitignore (recommended)

Create `.gitignore` in your repo:

```bash
# Build artifacts
*.o
dryer
build/
*.tar.gz

# Editor files
.vscode/
*.swp
*~

# OS files
.DS_Store
Thumbs.db
```

---

## Part 8: Troubleshooting

### SSH Connection Issues

```powershell
# If .local doesn't work:
ping dryer-pi.local

# Use IP directly:
ssh dryer@192.168.1.XXX

# Check SSH is enabled:
# Re-flash SD card with SSH enabled in Imager
```

### Git Authentication Issues

```bash
# Use Personal Access Token (PAT):
# 1. Create at: https://github.com/settings/tokens
# 2. Select scopes: repo (full access)
# 3. Use PAT as password when cloning

# Or use SSH keys:
ssh-keygen -t ed25519 -C "your@email.com"
cat ~/.ssh/id_ed25519.pub  # Add to GitHub SSH keys
git remote set-url origin git@github.com:YOUR_USERNAME/dryer-pi.git
```

### Build Fails

```bash
# Re-run setup
cd ~/dryer-pi
sudo ./setup.sh

# Check dependencies
dpkg -l | grep libsdl2
dpkg -l | grep pigpio
```

### Display Not Working

```bash
# Check HDMI config
cat /boot/config.txt | grep hdmi

# Should show:
# hdmi_group=2
# hdmi_mode=87
# hdmi_cvt=480 480 60 1 0 0 0
# hdmi_force_hotplug=1

# If not, re-run setup or manually add
```

---

## Part 9: Quick Reference

### Essential Commands

```bash
# SSH in
ssh dryer@dryer-pi.local

# Pull latest code
cd ~/dryer-pi && git pull

# Rebuild & restart
make clean && make && sudo make install && sudo systemctl restart dryer.service

# View logs
sudo journalctl -u dryer.service -f

# Test hardware
sudo ./test-hardware.sh

# Manual run (for debugging)
sudo systemctl stop dryer.service && sudo ./dryer
```

### File Locations

```
~/dryer-pi/                  # Git repository
/usr/local/bin/dryer         # Installed binary
/etc/systemd/system/dryer.service  # Service file
/boot/config.txt             # HDMI configuration
/dev/i2c-1                   # I2C bus
/dev/serial0                 # UART for MIDI
```

### Port Mappings

```
GPIO 2,3   : I2C (SDA, SCL)
GPIO 14    : UART TX (MIDI out)
GPIO 17    : Ball type switch
GPIO 22    : Moon gravity switch
GPIO 23    : Trigger out 1 (drum)
GPIO 24    : Trigger out 2 (vanes)
GPIO 27    : Lint trap switch
```

---

## Part 10: Advanced Tips

### Remote Development with VS Code

```powershell
# Install Remote-SSH extension in VS Code
# Connect to Pi: F1 → "Remote-SSH: Connect to Host"
# Enter: dryer@dryer-pi.local

# Now edit files directly on Pi!
# IntelliSense, debugging, terminal all work remotely
```

### Automated Deploy Script

Create `deploy.sh` in your repo:

```bash
#!/bin/bash
# deploy.sh - Quick deploy script

cd ~/dryer-pi
git pull
make clean && make
sudo make install
sudo systemctl restart dryer.service
echo "✓ Deployed and restarted"
sudo journalctl -u dryer.service -n 10
```

Then:
```bash
chmod +x deploy.sh
./deploy.sh  # One command to update everything!
```

### Backup Configuration

```bash
# Backup /boot/config.txt
sudo cp /boot/config.txt ~/dryer-pi/boot-config-backup.txt

# Backup service file
sudo cp /etc/systemd/system/dryer.service ~/dryer-pi/dryer-service-backup.service

# Commit to git
cd ~/dryer-pi
git add boot-config-backup.txt dryer-service-backup.service
git commit -m "Backup Pi configuration files"
git push
```

---

## Summary Workflow

```
┌─────────────┐
│   Windows   │
│  Dev Machine│
└──────┬──────┘
       │ git push
       ↓
┌─────────────┐
│   GitHub    │
│  Repository │
└──────┬──────┘
       │ git pull
       ↓
┌─────────────┐
│ Raspberry Pi│
│  dryer-pi   │
└─────────────┘
  make && run
```

**Typical development cycle:**
1. Edit code on Windows (VS Code, etc.)
2. `git commit && git push`
3. SSH to Pi: `git pull`
4. Rebuild: `make clean && make && sudo make install`
5. Test: `sudo systemctl restart dryer.service`
6. Repeat!

---

This workflow gives you:
- ✓ Version control
- ✓ Easy updates
- ✓ Backup of all code
- ✓ Professional development workflow
- ✓ No manual file copying
- ✓ Can develop from anywhere

Enjoy! 🎉
