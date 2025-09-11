#include "diffphc_core.h"
#include <getopt.h>
#include <iomanip>
#include <iostream>

class ShiwaDiffPHCCLI {
private:
    PHCConfig config;
    bool verbose = false;
    bool continuous = false;
    bool json_output = false;
    bool show_statistics = true;
    bool statistics_only = false;
    std::string output_file;

public:
    void printHelp() {
        std::cout
            << "ShiwaDiffPHC - Инструмент для измерения различий PHC (Протокол точного времени)\n"
            << "\nИспользование: shiwadiffphc [ОПЦИИ]\n"
            << "\nОсновные опции:\n"
            << "  -c, --count NUM     Количество итераций (по умолчанию: бесконечно)\n"
            << "  -l, --delay NUM     Задержка между итерациями в микросекундах (по умолчанию: 100000)\n"
            << "  -s, --samples NUM   Количество чтений PHC на измерение (по умолчанию: 10)\n"
            << "  -d, --device NUM    Добавить PTP устройство в список измерений (можно использовать несколько раз)\n"
            << "\nИнформация:\n"
            << "  -i, --info          Показать возможности PTP часов и выйти\n"
            << "  -L, --list          Список всех доступных PTP устройств и выход\n"
            << "  -h, --help          Отобразить эту справку и выйти\n"
            << "\nОпции вывода:\n"
            << "  -v, --verbose       Включить подробный вывод\n"
            << "  -q, --quiet         Подавить вывод прогресса\n"
            << "  -j, --json          Вывод результатов в формате JSON\n"
            << "  -o, --output FILE   Записать вывод в файл\n"
            << "\nРасширенные опции:\n"
            << "  --continuous        Запуск непрерывно (то же что -c 0)\n"
            << "  --csv               Вывод в формате CSV\n"
            << "  --precision NUM     Установить точность для временных различий (по умолчанию: 0)\n"
            << "\nСтатистические опции:\n"
            << "  --stats             Показать статистический анализ (по умолчанию: включено)\n"
            << "  --no-stats          Отключить показ статистики\n"
            << "  --stats-only        Показать только статистику без сырых данных\n"
            << "\nПримеры:\n"
            << "  shiwadiffphc -d 0 -d 1                    # Сравнить PTP устройства 0 и 1\n"
            << "  shiwadiffphc -c 100 -l 250000 -d 2 -d 0  # 100 итераций с задержкой 250мс\n"
            << "  shiwadiffphc -i                           # Показать информацию о PTP устройствах\n"
            << "  shiwadiffphc -L                           # Список доступных устройств\n"
            << "  shiwadiffphc -d 0 -d 1 --json -o out.json # Вывод JSON в файл\n"
            << "  shiwadiffphc -d 0 -d 1 --stats-only       # Только статистический анализ\n"
            << "  shiwadiffphc -d 0 -d 1 --no-stats         # Без статистики\n"
            << std::endl;
    }

    void printVersion() {
        std::cout << "ShiwaDiffPHC версия 1.2.0" << std::endl;
        std::cout << "Инструмент измерения различий протокола точного времени" << std::endl;
        std::cout << "Новинка: расширенный статистический анализ!" << std::endl;
    }

    void listDevices() {
        auto devices = DiffPHCCore::getAvailablePHCDevices();
        if (devices.empty()) {
            std::cout << "PTP устройства не найдены." << std::endl;
            return;
        }

        std::cout << "Доступные PTP устройства:" << std::endl;
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

        if (statistics_only) {
            outputStatisticsOnly(result);
        } else {
            outputResultsTable(result);
            if (show_statistics && !result.statistics.empty()) {
                outputStatistics(result);
            }
        }
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
                    int64_t diff = latest[idx++];
                    if (i == j) {
                        std::cout << "0\t";  // Same device = 0 difference
                    } else {
                        // Format difference in nanoseconds/microseconds
                        if (std::abs(diff) >= 1000) {
                            std::cout << std::fixed << std::setprecision(1) 
                                      << (diff / 1000.0) << "μs\t";
                        } else {
                            std::cout << diff << "ns\t";
                        }
                    }
                }
                std::cout << "\n";
            }
        }
        std::cout << std::endl;
    }

    void outputStatistics(const PHCResult& result) {
        const auto& devices = result.devices;
        const int numDev = devices.size();
        
        std::cout << "\n=== СТАТИСТИЧЕСКИЙ АНАЛИЗ ===" << std::endl;
        std::cout << "Количество измерений: " << result.differences.size() << std::endl;
        std::cout << std::endl;
        
        for (int i = 0; i < numDev; ++i) {
            for (int j = 0; j <= i; ++j) {
                if (i == j) continue; // Пропускаем диагональ (разность устройства с самим собой)
                
                const auto& stats = result.statistics[i][j];
                std::cout << "Пара устройств ptp" << devices[i] << " - ptp" << devices[j] << ":" << std::endl;
                
                // Format median
                if (std::abs(stats.median) >= 1000) {
                    std::cout << "  Медиана:           " << std::fixed << std::setprecision(1) 
                              << (stats.median / 1000.0) << " μс" << std::endl;
                } else {
                    std::cout << "  Медиана:           " << std::fixed << std::setprecision(1) 
                              << stats.median << " нс" << std::endl;
                }
                
                // Format mean
                if (std::abs(stats.mean) >= 1000) {
                    std::cout << "  Среднее:           " << std::fixed << std::setprecision(1) 
                              << (stats.mean / 1000.0) << " μс" << std::endl;
                } else {
                    std::cout << "  Среднее:           " << std::fixed << std::setprecision(1) 
                              << stats.mean << " нс" << std::endl;
                }
                
                // Format min/max
                if (std::abs(stats.minimum) >= 1000) {
                    std::cout << "  Минимум:           " << std::fixed << std::setprecision(1) 
                              << (stats.minimum / 1000.0) << " μс" << std::endl;
                } else {
                    std::cout << "  Минимум:           " << stats.minimum << " нс" << std::endl;
                }
                
                if (std::abs(stats.maximum) >= 1000) {
                    std::cout << "  Максимум:          " << std::fixed << std::setprecision(1) 
                              << (stats.maximum / 1000.0) << " μс" << std::endl;
                } else {
                    std::cout << "  Максимум:          " << stats.maximum << " нс" << std::endl;
                }
                
                // Format range
                if (stats.range >= 1000) {
                    std::cout << "  Размах:            " << std::fixed << std::setprecision(1) 
                              << (stats.range / 1000.0) << " μс" << std::endl;
                } else {
                    std::cout << "  Размах:            " << stats.range << " нс" << std::endl;
                }
                
                // Format stddev
                if (std::abs(stats.stddev) >= 1000) {
                    std::cout << "  Станд. отклонение: " << std::fixed << std::setprecision(1) 
                              << (stats.stddev / 1000.0) << " μс" << std::endl;
                } else {
                    std::cout << "  Станд. отклонение: " << std::fixed << std::setprecision(1) 
                              << stats.stddev << " нс" << std::endl;
                }
                
                std::cout << "  Измерений:         " << stats.count << std::endl;
                std::cout << std::endl;
            }
        }
    }

    void outputStatisticsOnly(const PHCResult& result) {
        const auto& devices = result.devices;
        const int numDev = devices.size();
        
        std::cout << "=== СТАТИСТИЧЕСКИЙ АНАЛИЗ ВРЕМЕННЫХ РАЗЛИЧИЙ ===" << std::endl;
        std::cout << "Количество измерений: " << result.differences.size() << std::endl;
        std::cout << std::endl;
        
        // Заголовок таблицы
        std::cout << std::left << std::setw(12) << "Пара" 
                  << std::setw(12) << "Медиана" 
                  << std::setw(12) << "Среднее"
                  << std::setw(12) << "Минимум"
                  << std::setw(12) << "Максимум"
                  << std::setw(12) << "Размах"
                  << std::setw(12) << "Станд.откл"
                  << std::setw(8) << "Счетчик" << std::endl;
        std::cout << std::string(90, '-') << std::endl;
        
        for (int i = 0; i < numDev; ++i) {
            for (int j = 0; j <= i; ++j) {
                if (i == j) continue;
                
                const auto& stats = result.statistics[i][j];
                std::cout << std::left << std::setw(12) 
                          << ("ptp" + std::to_string(devices[i]) + "-ptp" + std::to_string(devices[j]))
                          << std::setw(12) << std::fixed << std::setprecision(1) << stats.median
                          << std::setw(12) << std::fixed << std::setprecision(1) << stats.mean
                          << std::setw(12) << stats.minimum
                          << std::setw(12) << stats.maximum
                          << std::setw(12) << stats.range
                          << std::setw(12) << std::fixed << std::setprecision(1) << stats.stddev
                          << std::setw(8) << stats.count << std::endl;
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
            if (!statistics_only) {
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
            }
            
            // Добавить статистику если есть
            if (show_statistics && !result.statistics.empty()) {
                std::cout << "  \"statistics\": {\n";
                bool first_pair = true;
                const auto& devices = result.devices;
                const int numDev = devices.size();
                
                for (int i = 0; i < numDev; ++i) {
                    for (int j = 0; j <= i; ++j) {
                        if (i == j) continue;
                        
                        if (!first_pair) std::cout << ",\n";
                        first_pair = false;
                        
                        const auto& stats = result.statistics[i][j];
                        std::cout << "    \"ptp" << devices[i] << "-ptp" << devices[j] << "\": {\n";
                        std::cout << "      \"median\": " << stats.median << ",\n";
                        std::cout << "      \"mean\": " << stats.mean << ",\n";
                        std::cout << "      \"minimum\": " << stats.minimum << ",\n";
                        std::cout << "      \"maximum\": " << stats.maximum << ",\n";
                        std::cout << "      \"range\": " << stats.range << ",\n";
                        std::cout << "      \"stddev\": " << stats.stddev << ",\n";
                        std::cout << "      \"count\": " << stats.count << "\n";
                        std::cout << "    }";
                    }
                }
                std::cout << "\n  },\n";
            }
            
            std::cout << "  \"timestamp\": " << result.baseTimestamp << "\n";
        } else {
            std::cout << "  \"error\": \"" << result.error << "\"\n";
        }
        std::cout << "}\n";
    }

    void outputResultsCSV(const PHCResult& result) {
        const auto& devices = result.devices;
        const int numDev = devices.size();
        
        if (statistics_only) {
            // CSV заголовок для статистики
            std::cout << "pair,median,mean,minimum,maximum,range,stddev,count\n";
            
            // Данные статистики
            for (int i = 0; i < numDev; ++i) {
                for (int j = 0; j <= i; ++j) {
                    if (i == j) continue;
                    
                    const auto& stats = result.statistics[i][j];
                    std::cout << "ptp" << devices[i] << "-ptp" << devices[j] << ","
                              << stats.median << ","
                              << stats.mean << ","
                              << stats.minimum << ","
                              << stats.maximum << ","
                              << stats.range << ","
                              << stats.stddev << ","
                              << stats.count << "\n";
                }
            }
        } else {
            // CSV заголовок для измерений
            std::cout << "iteration,timestamp";
            
            for (int i = 0; i < numDev; ++i) {
                for (int j = 0; j <= i; ++j) {
                    std::cout << ",ptp" << devices[i] << "-ptp" << devices[j];
                }
            }
            std::cout << "\n";

            // Данные измерений
            for (size_t m = 0; m < result.differences.size(); ++m) {
                std::cout << m << "," << result.baseTimestamp;
                for (size_t d = 0; d < result.differences[m].size(); ++d) {
                    std::cout << "," << result.differences[m][d];
                }
                std::cout << "\n";
            }
            
            // Добавить статистику если включена
            if (show_statistics && !result.statistics.empty()) {
                std::cout << "\n# Статистический анализ\n";
                std::cout << "pair,median,mean,minimum,maximum,range,stddev,count\n";
                
                for (int i = 0; i < numDev; ++i) {
                    for (int j = 0; j <= i; ++j) {
                        if (i == j) continue;
                        
                        const auto& stats = result.statistics[i][j];
                        std::cout << "ptp" << devices[i] << "-ptp" << devices[j] << ","
                                  << stats.median << ","
                                  << stats.mean << ","
                                  << stats.minimum << ","
                                  << stats.maximum << ","
                                  << stats.range << ","
                                  << stats.stddev << ","
                                  << stats.count << "\n";
                    }
                }
            }
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
        [[maybe_unused]] bool csv_format = false; // TODO: implement CSV format support
        [[maybe_unused]] int precision = 0; // TODO: implement precision setting

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
            {"stats", 0, nullptr, 1005},
            {"no-stats", 0, nullptr, 1006},
            {"stats-only", 0, nullptr, 1007},
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
                case 1005: // --stats
                    show_statistics = true;
                    break;
                case 1006: // --no-stats
                    show_statistics = false;
                    break;
                case 1007: // --stats-only
                    statistics_only = true;
                    show_statistics = true;
                    break;
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

        // Check if PTP devices are available
        std::string ptp_error;
        if (!DiffPHCCore::checkPTPDevicesAvailable(ptp_error)) {
            std::cerr << "Error: " << ptp_error << std::endl;
            return 3;
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
            if (freopen(output_file.c_str(), "w", stdout) == nullptr) {
                std::cerr << "Error: failed to redirect output to file '" << output_file << "'" << std::endl;
                return 1;
            }
        }

        outputResults(result);
        
        return result.success ? 0 : 1;
    }
};

int main(int argc, char** argv) {
    ShiwaDiffPHCCLI cli;
    return cli.run(argc, argv);
}