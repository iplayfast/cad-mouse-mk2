#!/bin/bash
# CAD Mouse MK2 - Linux setup script
# Sets up spacenavd so the space mouse works in FreeCAD.
# Run once after flashing the firmware.

set -e

echo "Installing spacenavd..."
sudo apt install -y spacenavd

echo "Writing spacenavd config..."
sudo tee /etc/spnavrc > /dev/null << 'EOF'
device-id = 2886:0058
sensitivity = 0.5
dead-zone = 15
EOF

echo "Creating X11 autostart entry..."
mkdir -p ~/.config/autostart
cat > ~/.config/autostart/xhost-spacenavd.desktop << 'EOF'
[Desktop Entry]
Type=Application
Name=xhost for spacenavd
Exec=xhost +local:root
Hidden=false
X-GNOME-Autostart-enabled=true
EOF

echo "Enabling and starting spacenavd..."
sudo systemctl enable spacenavd
sudo systemctl restart spacenavd

echo "Granting X11 access for this session..."
xhost +local:root

echo ""
echo "Done. Plug in the CAD Mouse MK2 (leave it still for a couple of seconds"
echo "while it calibrates), then open FreeCAD and it should work."
echo "On future logins everything starts automatically."
