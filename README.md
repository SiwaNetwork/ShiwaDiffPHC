# DiffPHC

DiffPHC is a comprehensive tool for measuring time differences between Precision Time Protocol (PTP) devices in your system. It now features both enhanced command-line and graphical user interfaces for maximum usability.

## Features

### Core Functionality
- Measure time differences between multiple PTP devices
- High-precision timestamp analysis using PTP_SYS_OFFSET_EXTENDED
- Real-time monitoring capabilities
- Statistical analysis of clock drift

### User Interfaces

#### 1. Enhanced CLI Interface (`diffphc-cli`)
- **Rich command-line options** with long and short forms
- **Multiple output formats**: table, JSON, CSV
- **Auto-device detection** when no devices specified
- **Verbose logging** and debugging support
- **File output** capabilities
- **Device information** display

#### 2. Modern GUI Interface (`diffphc-gui`)
- **Intuitive graphical interface** built with Qt5
- **Real-time measurement display** with live tables
- **Device management** with automatic detection and refresh
- **Progress tracking** with visual indicators
- **Results export** in multiple formats
- **Configuration save/load** functionality
- **Built-in logging** and status monitoring

#### 3. Legacy CLI Interface (`diffphc`)
- Original command-line interface for backward compatibility

## Installation

### Dependencies

#### For CLI tools:
```bash
sudo apt-get install build-essential
```

#### For GUI tool (additional):
```bash
sudo apt-get install qtbase5-dev qt5-qmake
```

### Quick Install
```bash
# Install dependencies automatically
make install-deps

# Build all tools
make

# Install system-wide
sudo make install
```

### Manual Build
```bash
# Check dependencies
make check-deps

# Build specific targets
make diffphc-cli    # CLI only
make diffphc-gui    # GUI (requires Qt5)
make diffphc        # Legacy version
```

## Usage

### Enhanced CLI Interface

#### Basic Usage
```bash
# Auto-detect and compare first two devices
diffphc-cli

# Compare specific devices
diffphc-cli -d 0 -d 1

# Run 100 iterations with custom delay
diffphc-cli -c 100 -l 250000 -d 0 -d 1
```

#### Information Commands
```bash
# List all available PTP devices
diffphc-cli --list

# Show device capabilities
diffphc-cli --info

# Show device info for specific devices
diffphc-cli -i -d 0 -d 1
```

#### Output Options
```bash
# JSON output
diffphc-cli -d 0 -d 1 --json

# CSV output with file save
diffphc-cli -d 0 -d 1 --csv -o results.csv

# Verbose output
diffphc-cli -d 0 -d 1 --verbose
```

#### Advanced Options
```bash
# Continuous measurement
diffphc-cli -d 0 -d 1 --continuous

# Custom sampling
diffphc-cli -d 0 -d 1 -s 25 -l 50000

# Help and version
diffphc-cli --help
diffphc-cli --version
```

### GUI Interface

Launch the GUI application:
```bash
diffphc-gui
```

#### GUI Features:
1. **Device Selection**: Check boxes for available PTP devices
2. **Configuration Panel**: Set iterations, delay, and samples
3. **Real-time Control**: Start/stop measurements with progress tracking
4. **Results Display**: Live table updates with timestamp information
5. **Export Options**: Save results in CSV or JSON format
6. **Device Management**: Refresh device list and view device information
7. **Logging**: Built-in log panel for monitoring operations

#### GUI Requirements:
- Root privileges (sudo) to access PTP devices
- Qt5 libraries installed
- X11 or Wayland display server

### Legacy Interface

```bash
# Original usage (backward compatibility)
diffphc -c 100 -l 250000 -d 2 -d 0
diffphc -d 0 -d 1 -d 0
diffphc -i  # Show device info
```

## Command Line Options

### Enhanced CLI (`diffphc-cli`)

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-c NUM` | `--count NUM` | Number of iterations (0 = infinite) |
| `-l NUM` | `--delay NUM` | Delay between iterations (μs) |
| `-s NUM` | `--samples NUM` | Number of PHC reads per measurement |
| `-d NUM` | `--device NUM` | Add PTP device (repeatable) |
| `-i` | `--info` | Show PTP device information |
| `-L` | `--list` | List available PTP devices |
| `-v` | `--verbose` | Enable verbose output |
| `-q` | `--quiet` | Suppress progress output |
| `-j` | `--json` | Output in JSON format |
| `-o FILE` | `--output FILE` | Write output to file |
| | `--continuous` | Run continuously (same as -c 0) |
| | `--csv` | Output in CSV format |
| | `--version` | Show version information |
| `-h` | `--help` | Display help message |

## Output Formats

### Table Format (Default)
```
          ptp0    ptp1
ptp0      0       -1234
ptp1      1234    0
```

### JSON Format
```json
{
  "success": true,
  "devices": [0, 1],
  "measurements": [
    [0, -1234, 1234, 0]
  ],
  "timestamp": 1640995200000000000
}
```

### CSV Format
```csv
iteration,timestamp,ptp0-ptp0,ptp0-ptp1,ptp1-ptp0,ptp1-ptp1
0,1640995200000000000,0,-1234,1234,0
```

## System Requirements

- **Linux kernel** with PTP support
- **Root privileges** to access `/dev/ptp*` devices
- **PTP devices** available in system
- **C++17 compatible compiler**
- **Qt5 development libraries** (for GUI)

## Examples

### CLI Examples
```bash
# Quick device comparison
diffphc-cli -d 0 -d 1

# High-frequency monitoring
diffphc-cli -d 0 -d 1 -c 1000 -l 10000

# Export data for analysis
diffphc-cli -d 0 -d 1 -c 100 --csv -o measurement_data.csv

# Device discovery and info
diffphc-cli --list
diffphc-cli --info -d 0
```

### GUI Workflow
1. **Launch**: `sudo diffphc-gui`
2. **Select devices**: Check desired PTP devices
3. **Configure**: Set measurement parameters
4. **Start**: Click "Start Measurement"
5. **Monitor**: Watch real-time results in table
6. **Export**: Save results when complete

## Troubleshooting

### Common Issues

**Permission Denied**
- Solution: Run with sudo privileges
- Command: `sudo diffphc-cli` or `sudo diffphc-gui`

**No PTP Devices Found**
- Check: `ls /dev/ptp*`
- Ensure: PTP support in kernel and hardware

**Qt5 Not Found (GUI)**
- Install: `sudo apt-get install qtbase5-dev`
- Verify: `pkg-config --exists Qt5Core`

**Build Errors**
- Check dependencies: `make check-deps`
- Install missing: `make install-deps`

### Device Verification
```bash
# Check available devices
diffphc-cli --list

# Test device access
sudo diffphc-cli -d 0 --info

# Verify permissions
ls -la /dev/ptp*
```

## Development

### Building from Source
```bash
git clone <repository>
cd DiffPHC
make check-deps
make
```

### Project Structure
```
DiffPHC/
├── diffphc_core.h/.cpp     # Core measurement logic
├── diffphc_cli.cpp         # Enhanced CLI interface
├── diffphc_gui.h/.cpp      # Qt GUI interface
├── diffphc.cpp             # Legacy CLI interface
├── Makefile                # Build system
└── README.md               # Documentation
```

## License

Contributions to this Specification are made under the terms and conditions set forth in Open Web Foundation Contributor License Agreement ("OWF CLA 1.0") ("Contribution License") by Facebook.

## Version History

- **v1.1.0**: Added CLI and GUI interfaces, modular architecture
- **v1.0.0**: Original command-line tool
