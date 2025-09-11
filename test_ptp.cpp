#include <iostream>
#include <chrono>
#include <sys/time.h>

int64_t getCPUNow() {
    struct timespec ts = {};
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_nsec + ts.tv_sec * 1000000000LL;
}

int main() {
    std::cout << "Current system time: " << getCPUNow() << " nanoseconds" << std::endl;
    std::cout << "Current system time: " << (getCPUNow() / 1000000000.0) << " seconds since epoch" << std::endl;
    
    // Calculate years since epoch
    double years = (getCPUNow() / 1000000000.0) / (365.25 * 24 * 3600);
    std::cout << "Years since Unix epoch (1970): " << years << std::endl;
    
    return 0;
}
