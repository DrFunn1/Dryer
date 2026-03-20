#!/bin/bash

# ============================================================================
# Dryer Eurorack Module - Automated Setup Script
# For Raspberry Pi Zero 2W / Pi 4 / Pi 5
# ============================================================================

set -e  # Exit on error

echo "======================================"
echo "  Dryer Eurorack Setup Script"
echo "======================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: Please run as root (sudo ./setup.sh)"
    exit 1
fi

# Detect user
if [ -n "$SUDO_USER" ]; then
    ACTUAL_USER=$SUDO_USER
else
    ACTUAL_USER=$(whoami)
fi

echo "Running as user: $ACTUAL_USER"
echo ""

# ============================================================================
# STEP 1: System Update
# ============================================================================
echo "[1/8] Updating system packages..."
apt update
apt upgrade -y
echo "✓ System updated"
echo ""

# ============================================================================
# STEP 2: Install Dependencies
# ============================================================================
echo "[2/8] Installing dependencies..."
apt install -y \
    build-essential \
    cmake \
    git \
    libsdl2-dev \
    libgpiod-dev \
    gpiod \
    i2c-tools \
    vim \
    htop

echo "✓ Dependencies installed"
echo ""

# ============================================================================
# STEP 3: Enable I2C and UART
# ============================================================================
echo "[3/8] Configuring system interfaces..."

# Enable I2C
if ! grep -q "^dtparam=i2c_arm=on" /boot/config.txt; then
    echo "dtparam=i2c_arm=on" >> /boot/config.txt
    echo "  Enabled I2C"
fi

# Enable UART (disable login shell, enable hardware)
raspi-config nonint do_serial 1  # Disable serial console
raspi-config nonint do_serial_hw 0  # Enable serial hardware

# Optimize boot time
if ! grep -q "^dtoverlay=disable-bt" /boot/config.txt; then
    echo "dtoverlay=disable-bt" >> /boot/config.txt
    echo "  Disabled Bluetooth"
fi

# Set HDMI for 480x480 display ONLY
# Remove any existing HDMI config first
sed -i '/^hdmi_group=/d' /boot/config.txt
sed -i '/^hdmi_mode=/d' /boot/config.txt
sed -i '/^hdmi_cvt=/d' /boot/config.txt
sed -i '/^hdmi_force_hotplug=/d' /boot/config.txt

# Add clean round display config
cat >> /boot/config.txt << EOF

# Dryer 480x480 Round Display (ONLY display config)
hdmi_group=2
hdmi_mode=87
hdmi_cvt=480 480 60 1 0 0 0
hdmi_force_hotplug=1
disable_overscan=1
EOF
echo "  Configured HDMI for 480x480 display ONLY"

echo "✓ System interfaces configured"
echo ""

# ============================================================================
# STEP 4: Build Dryer
# ============================================================================
echo "[4/7] Building Dryer..."

# Use Makefile for simpler build
make clean 2>/dev/null || true
make -j4

if [ ! -f "dryer" ]; then
    echo "ERROR: Build failed - dryer executable not found"
    exit 1
fi

echo "✓ Build complete"
echo ""

# ============================================================================
# STEP 5: Install Binary
# ============================================================================
echo "[5/7] Installing dryer binary..."
make install
echo "✓ Binary installed to /usr/local/bin/dryer"
echo ""

# ============================================================================
# STEP 6: Install Systemd Service
# ============================================================================
echo "[6/7] Installing systemd service..."

cp dryer.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable dryer.service

echo "✓ Systemd service installed and enabled"
echo ""

# ============================================================================
# STEP 7: Disable Unnecessary Services (Boot Time Optimization)
# ============================================================================
echo "[7/7] Optimizing boot time..."

systemctl disable bluetooth.service 2>/dev/null || true
systemctl disable avahi-daemon.service 2>/dev/null || true
systemctl disable triggerhappy.service 2>/dev/null || true

echo "✓ Unnecessary services disabled"
echo ""

# ============================================================================
# Final Instructions
# ============================================================================
echo "======================================"
echo "  Setup Complete!"
echo "======================================"
echo ""
echo "Next steps:"
echo ""
echo "1. Reboot the system:"
echo "   sudo reboot"
echo ""
echo "2. After reboot, dryer will start automatically"
echo ""
echo "3. To check status:"
echo "   sudo systemctl status dryer.service"
echo ""
echo "4. To view logs:"
echo "   sudo journalctl -u dryer.service -f"
echo ""
echo "5. To manually start/stop:"
echo "   sudo systemctl start dryer.service"
echo "   sudo systemctl stop dryer.service"
echo ""
echo "6. To disable auto-start:"
echo "   sudo systemctl disable dryer.service"
echo ""
echo "Hardware connections:"
echo "  - I2C: GPIO 2 (SDA), GPIO 3 (SCL)"
echo "  - UART: GPIO 14 (TX) for MIDI"
echo "  - Switches: GPIO 17, 27, 22"
echo "  - Triggers: GPIO 23, 24"
echo ""
echo "Expected boot time: 8-15 seconds"
echo ""
echo "======================================"
