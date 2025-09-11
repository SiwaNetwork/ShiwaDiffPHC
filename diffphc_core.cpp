#include "diffphc_core.h"
#include <cmath>

std::string DiffPHCCore::getPHCFileName(int phc_index) {
    std::stringstream s;
    s << "/dev/ptp" << phc_index;
    return s.str();
}

int64_t DiffPHCCore::getCPUNow() {
    struct timespec ts = {};
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_nsec + ts.tv_sec * 1000000000LL;
}

int DiffPHCCore::openPHC(const std::string& pch_path) {
    int phc_fd = open(pch_path.c_str(), O_RDONLY);
    if (phc_fd >= 0) {
        // close fd at process exit
        auto flags = fcntl(phc_fd, F_GETFD);
        flags |= FD_CLOEXEC;
        fcntl(phc_fd, F_SETFD, flags);
    }
    return phc_fd;
}

bool DiffPHCCore::printClockInfo(int phc_index) {
    auto name = getPHCFileName(phc_index);
    int phc_fd = open(name.c_str(), O_RDONLY);
    if (phc_fd < 0) {
        return false;
    }
    std::cout << "PTP device " << name << std::endl;
    ptp_clock_caps caps = {};
    if (ioctl(phc_fd, PTP_CLOCK_GETCAPS, &caps)) {
        std::cout << "ioctl(PTP_CLOCK_GETCAPS) failed. errno: " << strerror(errno)
                  << std::endl;
    }

    struct ptp_sys_offset_extended sys_off_ext = {};
    sys_off_ext.n_samples = 1;
    bool supportOffsetExtended =
        !ioctl(phc_fd, PTP_SYS_OFFSET_EXTENDED, &sys_off_ext);

    std::cout << caps.max_adj
              << " maximum frequency adjustment in parts per billon.\n"
              << caps.n_ext_ts << " external time stamp channels.\n"
              << "PPS callback: " << (caps.pps ? "TRUE" : "FALSE") << "\n"
              << caps.n_pins << " input/output pins.\n"
              << "PTP_SYS_OFFSET_EXTENDED support: "
              << (supportOffsetExtended ? "TRUE" : "FALSE") << "\n"
              << std::endl;
    close(phc_fd);
    return true;
}

void DiffPHCCore::printClockInfoAll() {
    int phc_index = 0;
    while (printClockInfo(phc_index++))
        ;
    std::cout << (phc_index - 1) << " PTP device(s) found." << std::endl;
}

std::vector<int> DiffPHCCore::getAvailablePHCDevices() {
    std::vector<int> devices;
    int phc_index = 0;
    while (true) {
        auto name = getPHCFileName(phc_index);
        int phc_fd = open(name.c_str(), O_RDONLY);
        if (phc_fd < 0) {
            break;
        }
        devices.push_back(phc_index);
        close(phc_fd);
        phc_index++;
    }
    return devices;
}

bool DiffPHCCore::validateConfig(const PHCConfig& config, std::string& error) {
    // Validate count parameter
    if (config.count < 0) {
        error = "Invalid count parameter: must be >= 0 (0 = infinite)";
        return false;
    }
    
    // Validate delay parameter
    if (config.delay < 1) {
        error = "Invalid delay parameter: must be >= 1 microsecond";
        return false;
    }
    if (config.delay > 10000000) { // 10 seconds
        error = "Invalid delay parameter: must be <= 10,000,000 microseconds (10 seconds)";
        return false;
    }
    
    // Validate samples parameter
    if (config.samples < 1) {
        error = "Invalid samples parameter: must be >= 1";
        return false;
    }
    if (config.samples > PTP_MAX_SAMPLES) {
        error = "Invalid samples parameter: must be <= " + std::to_string(PTP_MAX_SAMPLES);
        return false;
    }
    
    // Validate devices
    if (config.devices.empty()) {
        error = "No devices specified";
        return false;
    }
    
    // Check for duplicate devices
    std::set<int> unique_devices(config.devices.begin(), config.devices.end());
    if (unique_devices.size() != config.devices.size()) {
        error = "Duplicate devices specified";
        return false;
    }
    
    // Check if devices exist and are accessible
    for (auto d : config.devices) {
        if (d < 0) {
            error = "Invalid device number: " + std::to_string(d) + " (must be >= 0)";
            return false;
        }
        
        auto name = getPHCFileName(d);
        int fd = openPHC(name);
        if (fd < 0) {
            error = "PTP device " + name + " not found or not accessible";
            return false;
        }
        close(fd);
    }
    
    return true;
}

bool DiffPHCCore::requiresRoot() {
    return geteuid() != 0;
}

bool DiffPHCCore::checkPTPDevicesAvailable(std::string& error) {
    auto devices = getAvailablePHCDevices();
    if (devices.empty()) {
        error = "No PTP devices found in the system. Please check:\n"
                "1. PTP support is enabled in kernel\n"
                "2. PTP hardware is connected\n"
                "3. PTP drivers are loaded\n"
                "4. Run 'ls /dev/ptp*' to check available devices";
        return false;
    }
    return true;
}

PHCResult DiffPHCCore::measurePHCDifferences(const PHCConfig& config) {
    PHCResult result;
    result.success = false;
    result.devices = config.devices;
    
    std::string error;
    if (!validateConfig(config, error)) {
        result.error = error;
        return result;
    }
    
    if (requiresRoot()) {
        result.error = "Root privileges required";
        return result;
    }
    
    std::vector<int> dev;
    for (auto d : config.devices) {
        auto name = getPHCFileName(d);
        int fd = openPHC(name);
        if (fd < 1) {
            result.error = "PTP device " + name + " open failed";
            return result;
        }
        dev.push_back(fd);
    }

    const int numDev = dev.size();
    std::vector<int64_t> ts(numDev);
    
    for (int c = 0; config.count == 0 || c < config.count; ++c) {
        int64_t baseTimestamp = getCPUNow();
        for (int d = 0; d < numDev; ++d) {
            int64_t now = getCPUNow();
            ts[d] = getPTPSysOffsetExtended(dev[d], config.samples) - (now - baseTimestamp);
        }
        
        std::vector<int64_t> differences;
        for (int i = 0; i < numDev; ++i) {
            for (int j = 0; j <= i; ++j) {
                differences.push_back(long(ts[i]) - long(ts[j]));
            }
        }
        
        result.differences.push_back(differences);
        result.baseTimestamp = baseTimestamp;
        
        if (config.count != 0 && c == config.count - 1) break;
        usleep(config.delay);
    }
    
    for (auto fd : dev) {
        close(fd);
    }
    
    result.success = true;
    
    // Рассчитать статистику, если есть измерения
    if (!result.differences.empty()) {
        calculateResultStatistics(result);
    }
    
    return result;
}

int64_t DiffPHCCore::getPTPSysOffsetExtended(int clkPTPid, int samples) {
    int64_t t0[PTP_MAX_SAMPLES];
    int64_t t1[PTP_MAX_SAMPLES];
    int64_t t2[PTP_MAX_SAMPLES];
    int64_t delay[PTP_MAX_SAMPLES];

    samples = std::min(PTP_MAX_SAMPLES, samples);

    struct ptp_sys_offset_extended sys_off = {};
    sys_off.n_samples = samples;
    if (ioctl(clkPTPid, PTP_SYS_OFFSET_EXTENDED, &sys_off)) {
        std::cerr << "ERR: ioctl(PTP_SYS_OFFSET_EXTENDED) failed : "
                  << strerror(errno) << std::endl;
        return 0;
    }

    int64_t mindelay = uint64_t(-1);
    for (int i = 0; i < samples; ++i) {
        t0[i] = sys_off.ts[i][0].nsec + 1000'000'000ULL * sys_off.ts[i][0].sec;
        t1[i] = sys_off.ts[i][1].nsec + 1000'000'000ULL * sys_off.ts[i][1].sec;
        t2[i] = sys_off.ts[i][2].nsec + 1000'000'000ULL * sys_off.ts[i][2].sec;
        delay[i] = t2[i] - t0[i];
        if (mindelay > delay[i]) {
            mindelay = delay[i];
        }
    }
    
    int count = 0;
    int64_t phcTotal = 0;
    int64_t sysTotal = 0;
    int64_t sysTime = 0;
    int64_t phcTime = 0;

    double delayTotal = 0.0;
    for (int i = 0; i < samples; ++i) {
        if (t2[i] < t0[i] || delay[i] > mindelay + PHCCallMaxDelay) {
            continue;
        }
        count++;
        if (count == 1) {
            sysTime = t0[i];
            phcTime = t1[i];
        }
        sysTotal += t0[i] - sysTime;
        phcTotal += t1[i] - phcTime;
        delayTotal += delay[i] / 2.0;
    }

    if (!count) {
        return 0;
    }

    sysTime += (sysTotal + count / 2) / count + int64_t(delayTotal / count);
    phcTime += (phcTotal + count / 2) / count;

    return getCPUNow() + phcTime - sysTime;
}

// Statistical analysis functions
double DiffPHCCore::calculateMedian(std::vector<int64_t> values) {
    if (values.empty()) return 0.0;
    
    std::sort(values.begin(), values.end());
    size_t n = values.size();
    
    if (n % 2 == 0) {
        return (values[n/2 - 1] + values[n/2]) / 2.0;
    } else {
        return values[n/2];
    }
}

double DiffPHCCore::calculateMean(const std::vector<int64_t>& values) {
    if (values.empty()) return 0.0;
    
    int64_t sum = 0;
    for (auto value : values) {
        sum += value;
    }
    return static_cast<double>(sum) / values.size();
}

double DiffPHCCore::calculateStdDev(const std::vector<int64_t>& values, double mean) {
    if (values.size() <= 1) return 0.0;
    
    double variance = 0.0;
    for (auto value : values) {
        double diff = value - mean;
        variance += diff * diff;
    }
    variance /= (values.size() - 1);
    return std::sqrt(variance);
}

PHCStatistics DiffPHCCore::calculateStatistics(const std::vector<int64_t>& values) {
    PHCStatistics stats = {};
    
    if (values.empty()) {
        stats.count = 0;
        return stats;
    }
    
    stats.count = values.size();
    
    // Найти минимум и максимум
    auto minmax = std::minmax_element(values.begin(), values.end());
    stats.minimum = *minmax.first;
    stats.maximum = *minmax.second;
    stats.range = stats.maximum - stats.minimum;
    
    // Рассчитать среднее
    stats.mean = calculateMean(values);
    
    // Рассчитать медиану
    stats.median = calculateMedian(values);
    
    // Рассчитать стандартное отклонение
    stats.stddev = calculateStdDev(values, stats.mean);
    
    return stats;
}

void DiffPHCCore::calculateResultStatistics(PHCResult& result) {
    if (!result.success || result.differences.empty()) {
        return;
    }
    
    const int numDev = result.devices.size();
    
    // Инициализировать массив статистики
    result.statistics.resize(numDev);
    for (int i = 0; i < numDev; ++i) {
        result.statistics[i].resize(i + 1);
    }
    
    // Собрать данные для каждой пары устройств
    std::vector<std::vector<std::vector<int64_t>>> pairData(numDev);
    for (int i = 0; i < numDev; ++i) {
        pairData[i].resize(i + 1);
    }
    
    // Заполнить данные из всех измерений
    for (const auto& measurement : result.differences) {
        int idx = 0;
        for (int i = 0; i < numDev; ++i) {
            for (int j = 0; j <= i; ++j) {
                pairData[i][j].push_back(measurement[idx++]);
            }
        }
    }
    
    // Рассчитать статистику для каждой пары
    for (int i = 0; i < numDev; ++i) {
        for (int j = 0; j <= i; ++j) {
            result.statistics[i][j] = calculateStatistics(pairData[i][j]);
        }
    }
}