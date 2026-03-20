#!/bin/bash

# Toggle between regular monitor and round 480x480 display

CONFIG="/boot/config.txt"

if [ "$EUID" -ne 0 ]; then 
    echo "Please run with sudo"
    exit 1
fi

echo "Current display mode:"
if grep -q "^hdmi_cvt=480 480" $CONFIG; then
    echo "  → Round 480x480 display"
    echo ""
    echo "Switching to regular monitor..."
    
    # Comment out round display settings
    sed -i 's/^hdmi_group=2/#hdmi_group=2/' $CONFIG
    sed -i 's/^hdmi_mode=87/#hdmi_mode=87/' $CONFIG
    sed -i 's/^hdmi_cvt=480 480/#hdmi_cvt=480 480/' $CONFIG
    
    echo "✓ Switched to regular monitor"
else
    echo "  → Regular monitor"
    echo ""
    echo "Switching to round 480x480 display..."
    
    # Uncomment round display settings
    sed -i 's/^#hdmi_group=2/hdmi_group=2/' $CONFIG
    sed -i 's/^#hdmi_mode=87/hdmi_mode=87/' $CONFIG
    sed -i 's/^#hdmi_cvt=480 480/hdmi_cvt=480 480/' $CONFIG
    
    # Make sure they exist if not already there
    if ! grep -q "hdmi_cvt=480 480" $CONFIG; then
        echo "" >> $CONFIG
        echo "# Round 480x480 display" >> $CONFIG
        echo "hdmi_group=2" >> $CONFIG
        echo "hdmi_mode=87" >> $CONFIG
        echo "hdmi_cvt=480 480 60 1 0 0 0" >> $CONFIG
    fi
    
    echo "✓ Switched to round display"
fi

echo ""
echo "Reboot required. Reboot now? (y/n)"
read -r response
if [[ "$response" =~ ^[Yy]$ ]]; then
    reboot
else
    echo "Remember to reboot: sudo reboot"
fi
