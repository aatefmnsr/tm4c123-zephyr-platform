#!/usr/bin/env bash
#
# Script to install persistent udev rules for the TI TM4C123G LaunchPad ICDI probe.
# Must be run with sudo: sudo ./setup-udev.sh
#

set -euo pipefail

if [ "$EUID" -ne 0 ]; then
    echo "[ERROR] Please run this script with sudo: sudo ./setup-udev.sh" >&2
    exit 1
fi

RULES_FILE="/etc/udev/rules.d/99-tm4c123.rules"

echo "[INFO] Creating persistent udev rule in $RULES_FILE ..."
cat << 'EOF' > "$RULES_FILE"
# TI Stellaris / Tiva C LaunchPad In-Circuit Debug Interface (TM4C123)
SUBSYSTEM=="usb", ATTR{idVendor}=="1cbe", ATTR{idProduct}=="00fd", MODE="0666"

# ICDI Virtual COM Port (Serial console)
KERNEL=="ttyACM*", ATTRS{idVendor}=="1cbe", ATTRS{idProduct}=="00fd", MODE="0666"
EOF

echo "[INFO] Reloading and triggering udev rules..."
udevadm control --reload-rules 2>/dev/null || true
udevadm trigger 2>/dev/null || true

# Also find and chmod any currently connected ICDI USB devices immediately
for dev in $(lsusb -d 1cbe:00fd | awk '{print "/dev/bus/usb/" $2 "/" substr($4, 1, length($4)-1)}'); do
    if [ -e "$dev" ]; then
        echo "[INFO] Setting immediate rw permissions on $dev ..."
        chmod 666 "$dev"
    fi
done

# Ensure user is in dialout group
TARGET_USER="${SUDO_USER:-$USER}"
if [ -n "$TARGET_USER" ] && [ "$TARGET_USER" != "root" ]; then
    echo "[INFO] Adding user '$TARGET_USER' to 'dialout' group..."
    usermod -aG dialout "$TARGET_USER" 2>/dev/null || true
fi

echo "[OK] Udev rules installed successfully!"
echo "     You can now run './build.sh -f' or './build.sh --flash-only' without sudo."
