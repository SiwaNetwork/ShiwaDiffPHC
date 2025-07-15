#!/bin/bash

# ShiwaDiffPHC GUI Launcher Script
# This script helps launch the ShiwaDiffPHC GUI with proper environment setup

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GUI_BINARY="$SCRIPT_DIR/shiwadiffphc-gui"

echo "ShiwaDiffPHC GUI Launcher"
echo "===================="

# Check if GUI binary exists
if [ ! -f "$GUI_BINARY" ]; then
    echo "Error: shiwadiffphc-gui not found at $GUI_BINARY"
    echo "Please build the GUI first with: make shiwadiffphc-gui"
    exit 1
fi

# Check if we're running as root
if [ "$EUID" -ne 0 ]; then
    echo "Warning: ShiwaDiffPHC requires root privileges to access PTP devices."
    echo "Attempting to restart with sudo..."
    exec sudo "$0" "$@"
fi

# Check for X11/Wayland display
if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
    echo "Error: No display server found."
    echo "Make sure you're running in a graphical environment (X11 or Wayland)."
    exit 1
fi

# Check for PTP devices
PTP_DEVICES=$(ls /dev/ptp* 2>/dev/null | wc -l)
if [ "$PTP_DEVICES" -eq 0 ]; then
    echo "Warning: No PTP devices found (/dev/ptp*)."
    echo "The GUI will still start, but no devices will be available for measurement."
    echo ""
fi

echo "Starting ShiwaDiffPHC GUI..."
echo "Found $PTP_DEVICES PTP device(s)"
echo ""

# Launch the GUI
exec "$GUI_BINARY" "$@"