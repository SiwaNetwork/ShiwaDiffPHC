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
#include <set>
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

struct PHCStatistics {
    double median;          // Медиана
    int64_t minimum;        // Минимум
    int64_t maximum;        // Максимум
    double mean;            // Среднее значение
    double stddev;          // Стандартное отклонение
    int64_t range;          // Размах (max - min)
    size_t count;           // Количество измерений
};

struct PHCResult {
    std::vector<int> devices;
    std::vector<std::vector<int64_t>> differences;
    int64_t baseTimestamp;
    bool success;
    std::string error;
    
    // Статистика по парам устройств
    std::vector<std::vector<PHCStatistics>> statistics;
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
    static bool checkPTPDevicesAvailable(std::string& error);
    
    // Statistical analysis
    static PHCStatistics calculateStatistics(const std::vector<int64_t>& values);
    static void calculateResultStatistics(PHCResult& result);
    static double calculateMedian(std::vector<int64_t> values);
    static double calculateMean(const std::vector<int64_t>& values);
    static double calculateStdDev(const std::vector<int64_t>& values, double mean);
};

#endif // DIFFPHC_CORE_H