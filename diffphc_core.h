#ifndef DIFFPHC_CORE_H
#define DIFFPHC_CORE_H

#include <ctype.h>
#include <fcntl.h>
#include <linux/ptp_clock.h>
#include <linux/sockios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

struct PHCConfig {
    int count = 0;
    int delay = 100000;
    int samples = 10;
    bool info = false;
    bool debug = false;
    std::vector<int> devices;
};

struct PHCResult {
    std::vector<int> devices;
    std::vector<std::vector<int64_t>> differences;
    int64_t baseTimestamp;
    bool success;
    std::string error;
};

class DiffPHCCore {
public:
    static const int MaxAttempts = 5;
    static const int64_t TAIOffset = 37'000'000;
    static const int64_t PHCCallMaxDelay = 100'000;

    // Core functionality
    static std::string getPHCFileName(int phc_index);
    static int64_t getCPUNow();
    static int openPHC(const std::string& pch_path);
    static bool printClockInfo(int phc_index);
    static void printClockInfoAll();
    static int64_t getPTPSysOffsetExtended(int clkPTPid, int samples);
    
    // High level operations
    static PHCResult measurePHCDifferences(const PHCConfig& config);
    static std::vector<int> getAvailablePHCDevices();
    static bool validateConfig(const PHCConfig& config, std::string& error);
    static bool requiresRoot();
};

#endif // DIFFPHC_CORE_H