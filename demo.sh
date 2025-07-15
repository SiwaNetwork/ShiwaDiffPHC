#!/bin/bash

# DiffPHC Demonstration Script
# This script demonstrates the capabilities of the enhanced DiffPHC tools

echo "=================================="
echo "DiffPHC Enhanced Demo"
echo "=================================="
echo ""

# Check if binaries exist
echo "1. Checking available binaries..."
if [ -f "./diffphc" ]; then
    echo "   ✓ Legacy CLI (diffphc) - Available"
else
    echo "   ✗ Legacy CLI (diffphc) - Not found"
fi

if [ -f "./diffphc-cli" ]; then
    echo "   ✓ Enhanced CLI (diffphc-cli) - Available"
else
    echo "   ✗ Enhanced CLI (diffphc-cli) - Not found"
fi

if [ -f "./diffphc-gui" ]; then
    echo "   ✓ GUI (diffphc-gui) - Available"
else
    echo "   ✗ GUI (diffphc-gui) - Not built (requires Qt5)"
fi
echo ""

# Show version information
echo "2. Version Information:"
if [ -f "./diffphc-cli" ]; then
    ./diffphc-cli --version
fi
echo ""

# Device discovery
echo "3. PTP Device Discovery:"
if [ -f "./diffphc-cli" ]; then
    ./diffphc-cli --list
else
    echo "   Enhanced CLI not available"
fi
echo ""

# Show capabilities comparison
echo "4. Feature Comparison:"
echo ""
echo "   Legacy CLI (diffphc):"
echo "   - Basic command-line interface"
echo "   - Simple output format"
echo "   - Original functionality"
echo ""
echo "   Enhanced CLI (diffphc-cli):"
echo "   - Rich command-line options"
echo "   - Multiple output formats (Table, JSON, CSV)"
echo "   - Auto-device detection"
echo "   - Verbose logging"
echo "   - File output support"
echo ""
echo "   GUI (diffphc-gui):"
echo "   - Graphical user interface"
echo "   - Real-time measurement display"
echo "   - Device management"
echo "   - Progress tracking"
echo "   - Results export"
echo ""

# Usage examples
echo "5. Usage Examples:"
echo ""
echo "   List available devices:"
echo "   $ ./diffphc-cli --list"
echo ""
echo "   Show device information:"
echo "   $ ./diffphc-cli --info"
echo ""
echo "   Compare two devices (if available):"
echo "   $ sudo ./diffphc-cli -d 0 -d 1 -c 10"
echo ""
echo "   Export data to CSV:"
echo "   $ sudo ./diffphc-cli -d 0 -d 1 -c 5 --csv -o results.csv"
echo ""
echo "   Launch GUI (if built):"
echo "   $ sudo ./launch_gui.sh"
echo "   or"
echo "   $ sudo ./diffphc-gui"
echo ""

# System requirements
echo "6. System Requirements:"
echo ""
echo "   For CLI tools:"
echo "   - Linux with PTP support"
echo "   - Root privileges for device access"
echo "   - C++17 compatible compiler"
echo ""
echo "   For GUI (additional):"
echo "   - Qt5 development libraries"
echo "   - X11 or Wayland display server"
echo ""

# Installation
echo "7. Installation Commands:"
echo ""
echo "   Install dependencies:"
echo "   $ make install-deps"
echo ""
echo "   Build all tools:"
echo "   $ make"
echo ""
echo "   Install system-wide:"
echo "   $ sudo make install"
echo ""

echo "=================================="
echo "Demo completed!"
echo ""
echo "To try the tools:"
echo "1. Check for PTP devices: ls /dev/ptp*"
echo "2. Run with root privileges: sudo ./diffphc-cli --list"
echo "3. For GUI: sudo ./launch_gui.sh (if built)"
echo "=================================="