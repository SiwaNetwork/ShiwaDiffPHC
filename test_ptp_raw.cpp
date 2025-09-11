#include <iostream>
#include <chrono>
#include <sys/time.h>
#include <fcntl.h>
#include <linux/ptp_clock.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

int64_t getCPUNow() {
    struct timespec ts = {};
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_nsec + ts.tv_sec * 1000000000LL;
}

int64_t getPTPSysOffsetExtended(int clkPTPid, int samples) {
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
        if (t2[i] < t0[i] || delay[i] > mindelay + 100'000) {
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

int main() {
    std::cout << "System time: " << getCPUNow() << " ns" << std::endl;
    
    int fd0 = open("/dev/ptp0", O_RDONLY);
    if (fd0 < 0) {
        std::cerr << "Cannot open /dev/ptp0" << std::endl;
        return 1;
    }
    
    int fd1 = open("/dev/ptp2", O_RDONLY);  // Используем PTP2 вместо PTP1
    if (fd1 < 0) {
        std::cerr << "Cannot open /dev/ptp2" << std::endl;
        close(fd0);
        return 1;
    }
    
    int64_t ptp0_time = getPTPSysOffsetExtended(fd0, 10);
    int64_t ptp1_time = getPTPSysOffsetExtended(fd1, 10);
    
    std::cout << "PTP0 time: " << ptp0_time << " ns" << std::endl;
    std::cout << "PTP2 time: " << ptp1_time << " ns" << std::endl;
    
    int64_t diff_ns = ptp1_time - ptp0_time;
    std::cout << "\n=== АНАЛИЗ РАЗНОСТИ ===" << std::endl;
    std::cout << "Разность: " << diff_ns << " наносекунд" << std::endl;
    std::cout << "Разность: " << (diff_ns / 1000.0) << " микросекунд" << std::endl;
    std::cout << "Разность: " << (diff_ns / 1000000.0) << " миллисекунд" << std::endl;
    std::cout << "Разность: " << (diff_ns / 1000000000.0) << " секунд" << std::endl;
    
    // Проверим, что это действительно наносекунды
    std::cout << "\n=== ПРОВЕРКА ЕДИНИЦ ===" << std::endl;
    std::cout << "Системное время: " << getCPUNow() << " ns" << std::endl;
    std::cout << "Системное время: " << (getCPUNow() / 1000000000.0) << " секунд с эпохи" << std::endl;
    
    close(fd0);
    close(fd1);
    
    return 0;
}
