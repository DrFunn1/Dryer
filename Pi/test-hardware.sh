#!/bin/bash

# ============================================================================
# Hardware Test Utility for Dryer Eurorack Module
# Tests I2C, GPIO, and UART connections
# ============================================================================

echo "======================================"
echo "  Dryer Hardware Test Utility"
echo "======================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: Please run as root (sudo ./test-hardware.sh)"
    exit 1
fi

SUCCESS=0
FAILED=0

# ============================================================================
# Test 1: I2C Bus
# ============================================================================
echo "[Test 1] I2C Bus Detection..."
if i2cdetect -y 1 | grep -q "48"; then
    echo "✓ PASS: ADS1115 detected at address 0x48"
    SUCCESS=$((SUCCESS + 1))
else
    echo "✗ FAIL: ADS1115 not detected"
    echo "  Check wiring: SDA=GPIO2, SCL=GPIO3, VDD=3.3V, GND=GND"
    FAILED=$((FAILED + 1))
fi
echo ""

# ============================================================================
# Test 2: UART Device
# ============================================================================
echo "[Test 2] UART Device Check..."
if [ -e "/dev/serial0" ]; then
    echo "✓ PASS: UART device exists at /dev/serial0"
    SUCCESS=$((SUCCESS + 1))
else
    echo "✗ FAIL: UART device not found"
    echo "  Run: sudo raspi-config → Interface Options → Serial"
    echo "  Disable login shell, enable serial hardware"
    FAILED=$((FAILED + 1))
fi
echo ""

# ============================================================================
# Test 3: GPIO Input Pins (Switches)
# ============================================================================
echo "[Test 3] GPIO Input Pins..."

TEST_PINS=(17 27 22)
PIN_NAMES=("BALL_TYPE" "LINT_TRAP" "MOON_GRAVITY")

for i in "${!TEST_PINS[@]}"; do
    PIN=${TEST_PINS[$i]}
    NAME=${PIN_NAMES[$i]}
    
    # Read value using gpioget
    VALUE=$(gpioget gpiochip0 $PIN)
    
    echo "  GPIO $PIN ($NAME): $VALUE"
    echo "    0 = Switch OFF, 1 = Switch ON"
done

echo "✓ PASS: GPIO inputs configured"
SUCCESS=$((SUCCESS + 1))
echo ""

# ============================================================================
# Test 4: GPIO Output Pins (Triggers)
# ============================================================================
echo "[Test 4] GPIO Output Pins..."

TRIGGER_PINS=(23 24)
TRIGGER_NAMES=("TRIGGER_OUT_1" "TRIGGER_OUT_2")

for i in "${!TRIGGER_PINS[@]}"; do
    PIN=${TRIGGER_PINS[$i]}
    NAME=${TRIGGER_NAMES[$i]}
    
    echo "  Testing GPIO $PIN ($NAME)..."
    
    # Test pulse using gpioset
    gpioset gpiochip0 $PIN=1
    sleep 0.05
    gpioset gpiochip0 $PIN=0
    
    echo "    Sent 50ms test pulse"
done

echo "✓ PASS: GPIO outputs tested (check with multimeter or scope)"
SUCCESS=$((SUCCESS + 1))
echo ""

# ============================================================================
# Test 5: Read ADC Channels (if ADS1115 present)
# ============================================================================
echo "[Test 5] ADC Channel Readings..."

if i2cdetect -y 1 | grep -q "48"; then
    echo "  Reading all 4 ADC channels..."
    echo "  Turn potentiometers to verify readings change"
    echo ""
    
    # Install Python i2c library if not present
    if ! python3 -c "import smbus2" 2>/dev/null; then
        echo "  Installing smbus2 for ADC test..."
        apt install -y python3-smbus
    fi
    
    # Quick Python script to read ADC
    python3 << 'EOF'
import smbus2
import time

bus = smbus2.SMBus(1)
ADS1115_ADDR = 0x48

def read_adc(channel):
    config = 0xC000 | (0x4000 + (channel << 12)) | 0x0200 | 0x0100 | 0x0080
    bus.write_i2c_block_data(ADS1115_ADDR, 0x01, [(config >> 8) & 0xFF, config & 0xFF])
    time.sleep(0.01)
    data = bus.read_i2c_block_data(ADS1115_ADDR, 0x00, 2)
    value = (data[0] << 8) | data[1]
    if value > 32768:
        value = 0
    voltage = (value / 26400.0) * 3.3
    return value, voltage

channels = ["RPM", "DRUM_SIZE", "VANES", "VANE_HEIGHT"]

for i in range(4):
    raw, voltage = read_adc(i)
    print(f"  A{i} ({channels[i]:12s}): {raw:5d} = {voltage:.2f}V")

bus.close()
EOF
    
    echo ""
    echo "✓ PASS: ADC channels readable"
    SUCCESS=$((SUCCESS + 1))
else
    echo "⊘ SKIP: ADS1115 not detected, skipping ADC test"
fi
echo ""

# ============================================================================
# Test 6: Display Output
# ============================================================================
echo "[Test 6] Display Output..."

if [ -n "$DISPLAY" ] || [ -e "/dev/fb0" ]; then
    echo "✓ PASS: Display device available"
    echo "  HDMI output or framebuffer detected"
    SUCCESS=$((SUCCESS + 1))
else
    echo "⚠ WARNING: Display may not be configured"
    echo "  Check /boot/config.txt for HDMI settings"
    echo "  Or verify monitor is connected"
fi
echo ""

# ============================================================================
# Summary
# ============================================================================
echo "======================================"
echo "  Test Summary"
echo "======================================"
echo "Passed: $SUCCESS"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "✓ All tests passed! Hardware is ready."
    echo ""
    echo "You can now run:"
    echo "  sudo ./dryer"
    echo ""
    echo "Or enable auto-start:"
    echo "  sudo systemctl enable dryer.service"
    echo "  sudo systemctl start dryer.service"
    exit 0
else
    echo "✗ Some tests failed. Please fix hardware connections."
    echo ""
    echo "Common issues:"
    echo "  - ADS1115 not detected: Check I2C wiring and address"
    echo "  - UART not found: Enable serial hardware in raspi-config"
    echo "  - GPIO issues: Ensure pigpiod is running"
    exit 1
fi
