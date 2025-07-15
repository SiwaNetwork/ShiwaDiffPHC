#include "diffphc_core.h"
#include <getopt.h>
#include <iomanip>

class DiffPHCCLI {
private:
    PHCConfig config;
    bool verbose = false;
    bool continuous = false;
    bool json_output = false;
    std::string output_file;

public:
    void printHelp() {
        std::cout
            << "DiffPHC - Tool for measuring PHC (Precision Time Protocol) differences\n"
            << "\nUsage: diffphc [OPTIONS]\n"
            << "\nBasic Options:\n"
            << "  -c, --count NUM     Number of iterations (default: infinite)\n"
            << "  -l, --delay NUM     Delay between iterations in microseconds (default: 100000)\n"
            << "  -s, --samples NUM   Number of PHC reads per measurement (default: 10)\n"
            << "  -d, --device NUM    Add PTP device to measurement list (can be used multiple times)\n"
            << "\nInformation:\n"
            << "  -i, --info          Show PTP clock capabilities and exit\n"
            << "  -L, --list          List all available PTP devices and exit\n"
            << "  -h, --help          Display this help and exit\n"
            << "\nOutput Options:\n"
            << "  -v, --verbose       Enable verbose output\n"
            << "  -q, --quiet         Suppress progress output\n"
            << "  -j, --json          Output results in JSON format\n"
            << "  -o, --output FILE   Write output to file\n"
            << "\nAdvanced Options:\n"
            << "  --continuous        Run continuously (same as -c 0)\n"
            << "  --csv               Output in CSV format\n"
            << "  --precision NUM     Set precision for time differences (default: 0)\n"
            << "\nExamples:\n"
            << "  diffphc -d 0 -d 1                    # Compare PTP devices 0 and 1\n"
            << "  diffphc -c 100 -l 250000 -d 2 -d 0  # 100 iterations with 250ms delay\n"
            << "  diffphc -i                           # Show PTP device info\n"
            << "  diffphc -L                           # List available devices\n"
            << "  diffphc -d 0 -d 1 --json -o out.json # JSON output to file\n"
            << std::endl;
    }

    void printVersion() {
        std::cout << "DiffPHC version 1.1.0" << std::endl;
        std::cout << "Precision Time Protocol difference measurement tool" << std::endl;
    }

    void listDevices() {
        auto devices = DiffPHCCore::getAvailablePHCDevices();
        if (devices.empty()) {
            std::cout << "No PTP devices found." << std::endl;
            return;
        }

        std::cout << "Available PTP devices:" << std::endl;
        for (int device : devices) {
            std::cout << "  /dev/ptp" << device;
            
            // Try to get additional info
            auto name = DiffPHCCore::getPHCFileName(device);
            int phc_fd = open(name.c_str(), O_RDONLY);
            if (phc_fd >= 0) {
                ptp_clock_caps caps = {};
                if (!ioctl(phc_fd, PTP_CLOCK_GETCAPS, &caps)) {
                    struct ptp_sys_offset_extended sys_off_ext = {};
                    sys_off_ext.n_samples = 1;
                    bool supportOffsetExtended = !ioctl(phc_fd, PTP_SYS_OFFSET_EXTENDED, &sys_off_ext);
                    std::cout << " (ext_ts: " << caps.n_ext_ts 
                              << ", pins: " << caps.n_pins
                              << ", pps: " << (caps.pps ? "yes" : "no")
                              << ", offset_ext: " << (supportOffsetExtended ? "yes" : "no") << ")";
                }
                close(phc_fd);
            }
            std::cout << std::endl;
        }
    }

    void outputResults(const PHCResult& result, bool csv_format = false) {
        if (!result.success) {
            std::cerr << "Error: " << result.error << std::endl;
            return;
        }

        if (json_output) {
            outputResultsJSON(result);
            return;
        }

        if (csv_format) {
            outputResultsCSV(result);
            return;
        }

        outputResultsTable(result);
    }

    void outputResultsTable(const PHCResult& result) {
        const auto& devices = result.devices;
        const int numDev = devices.size();

        // Header
        std::cout << "          ";
        for (int i = 0; i < numDev; ++i) {
            std::cout << "ptp" << devices[i] << "\t";
        }
        std::cout << "\n";

        // Matrix output for latest measurement
        if (!result.differences.empty()) {
            const auto& latest = result.differences.back();
            int idx = 0;
            for (int i = 0; i < numDev; ++i) {
                std::cout << "ptp" << devices[i] << "\t";
                for (int j = 0; j <= i; ++j) {
                    std::cout << latest[idx++] << "\t";
                }
                std::cout << "\n";
            }
        }
        std::cout << std::endl;
    }

    void outputResultsJSON(const PHCResult& result) {
        std::cout << "{\n";
        std::cout << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
        std::cout << "  \"devices\": [";
        for (size_t i = 0; i < result.devices.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result.devices[i];
        }
        std::cout << "],\n";
        
        if (result.success) {
            std::cout << "  \"measurements\": [\n";
            for (size_t m = 0; m < result.differences.size(); ++m) {
                if (m > 0) std::cout << ",\n";
                std::cout << "    [";
                for (size_t d = 0; d < result.differences[m].size(); ++d) {
                    if (d > 0) std::cout << ", ";
                    std::cout << result.differences[m][d];
                }
                std::cout << "]";
            }
            std::cout << "\n  ],\n";
            std::cout << "  \"timestamp\": " << result.baseTimestamp << "\n";
        } else {
            std::cout << "  \"error\": \"" << result.error << "\"\n";
        }
        std::cout << "}\n";
    }

    void outputResultsCSV(const PHCResult& result) {
        // CSV header
        std::cout << "iteration,timestamp";
        const auto& devices = result.devices;
        const int numDev = devices.size();
        
        for (int i = 0; i < numDev; ++i) {
            for (int j = 0; j <= i; ++j) {
                std::cout << ",ptp" << devices[i] << "-ptp" << devices[j];
            }
        }
        std::cout << "\n";

        // Data rows
        for (size_t m = 0; m < result.differences.size(); ++m) {
            std::cout << m << "," << result.baseTimestamp;
            for (size_t d = 0; d < result.differences[m].size(); ++d) {
                std::cout << "," << result.differences[m][d];
            }
            std::cout << "\n";
        }
    }

    int optArgToInt() {
        try {
            return std::stoi(optarg);
        } catch (...) {
            std::cerr << "Error: invalid argument '" << optarg << "'" << std::endl;
            exit(-1);
        }
    }

    int parseArgs(int argc, char** argv) {
        bool csv_format = false;
        int precision = 0;

        struct option longopts[] = {
            {"count", 1, nullptr, 'c'},
            {"delay", 1, nullptr, 'l'},
            {"samples", 1, nullptr, 's'},
            {"device", 1, nullptr, 'd'},
            {"info", 0, nullptr, 'i'},
            {"list", 0, nullptr, 'L'},
            {"help", 0, nullptr, 'h'},
            {"verbose", 0, nullptr, 'v'},
            {"quiet", 0, nullptr, 'q'},
            {"json", 0, nullptr, 'j'},
            {"output", 1, nullptr, 'o'},
            {"continuous", 0, nullptr, 1001},
            {"csv", 0, nullptr, 1002},
            {"precision", 1, nullptr, 1003},
            {"version", 0, nullptr, 1004},
            {0, 0, 0, 0}
        };

        int c;
        while ((c = getopt_long(argc, argv, "c:l:d:s:iLhvqjo:", longopts, NULL)) != -1) {
            switch (c) {
                case 'd':
                    config.devices.push_back(optArgToInt());
                    break;
                case 'c':
                    config.count = optArgToInt();
                    break;
                case 'l':
                    config.delay = optArgToInt();
                    break;
                case 's':
                    config.samples = optArgToInt();
                    break;
                case 'h':
                    printHelp();
                    return 0;
                case 'v':
                    verbose = true;
                    config.debug = true;
                    break;
                case 'q':
                    verbose = false;
                    break;
                case 'j':
                    json_output = true;
                    break;
                case 'o':
                    output_file = optarg;
                    break;
                case 'i':
                    config.info = true;
                    break;
                case 'L':
                    listDevices();
                    return 0;
                case 1001: // --continuous
                    continuous = true;
                    config.count = 0;
                    break;
                case 1002: // --csv
                    csv_format = true;
                    break;
                case 1003: // --precision
                    precision = optArgToInt();
                    break;
                case 1004: // --version
                    printVersion();
                    return 0;
                case 0:
                    break;
                case '?':
                default:
                    printHelp();
                    return -1;
            }
        }

        if (config.info) {
            if (config.devices.empty()) {
                DiffPHCCore::printClockInfoAll();
            } else {
                for (auto d : config.devices) {
                    if (!DiffPHCCore::printClockInfo(d)) {
                        std::cerr << "Error: device /dev/ptp" << d << " open failed" << std::endl;
                    }
                }
            }
            return 0;
        }

        // Auto-detect devices if none specified
        if (config.devices.empty()) {
            auto available = DiffPHCCore::getAvailablePHCDevices();
            if (available.size() >= 2) {
                config.devices.push_back(available[0]);
                config.devices.push_back(available[1]);
                if (verbose) {
                    std::cout << "Auto-detected devices: ptp" << available[0] 
                              << " and ptp" << available[1] << std::endl;
                }
            } else {
                std::cerr << "Error: No PTP devices specified and auto-detection failed" << std::endl;
                std::cerr << "Use -L to list available devices" << std::endl;
                return -1;
            }
        }

        return 1; // Continue execution
    }

    int run(int argc, char** argv) {
        int parse_result = parseArgs(argc, argv);
        if (parse_result <= 0) {
            return -parse_result;
        }

        if (DiffPHCCore::requiresRoot()) {
            std::cerr << "Error: Root privileges required to access PTP devices" << std::endl;
            return 2;
        }

        if (verbose) {
            std::cout << "Configuration:" << std::endl;
            std::cout << "  Iterations: " << (config.count == 0 ? "infinite" : std::to_string(config.count)) << std::endl;
            std::cout << "  Delay: " << config.delay << " μs" << std::endl;
            std::cout << "  Samples: " << config.samples << std::endl;
            std::cout << "  Devices: ";
            for (auto d : config.devices) {
                std::cout << "ptp" << d << " ";
            }
            std::cout << std::endl << std::endl;
        }

        auto result = DiffPHCCore::measurePHCDifferences(config);
        
        if (!output_file.empty()) {
            freopen(output_file.c_str(), "w", stdout);
        }

        outputResults(result);
        
        return result.success ? 0 : 1;
    }
};

int main(int argc, char** argv) {
    DiffPHCCLI cli;
    return cli.run(argc, argv);
}