#include "advanced_analysis.h"
#include "diffphc_core.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <sstream>
#include <iomanip>

// Trend Analysis Implementation
TrendAnalysis AdvancedAnalysis::analyzeTrend(const std::vector<int64_t>& values, const std::vector<int64_t>& timestamps) {
    TrendAnalysis result;
    
    if (values.size() < 2) {
        result.slope = 0.0;
        result.intercept = 0.0;
        result.r_squared = 0.0;
        result.correlation = 0.0;
        result.p_value = 1.0;
        result.trend_type = "insufficient_data";
        result.is_significant = false;
        return result;
    }
    
    // Generate normalized timestamps (0, 1, 2, 3, ...)
    std::vector<double> x(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        x[i] = static_cast<double>(i);
    }
    
    // Convert values to double and normalize
    std::vector<double> y = convertToDouble(values);
    
    // Normalize values to prevent overflow
    if (!y.empty()) {
        double mean_y = calculateMean(y);
        for (size_t i = 0; i < y.size(); ++i) {
            y[i] -= mean_y; // Center around zero
        }
    }
    
    // Calculate linear regression
    result.r_squared = calculateLinearRegression(x, y, result.slope, result.intercept);
    result.correlation = calculateCorrelation(x, y);
    
    // Determine trend type
    if (std::abs(result.slope) < 1e-9) {
        result.trend_type = "stable";
    } else if (result.slope > 0) {
        result.trend_type = "increasing";
    } else {
        result.trend_type = "decreasing";
    }
    
    // Simple significance test (can be improved with proper statistical tests)
    result.is_significant = (std::abs(result.correlation) > 0.5) && (values.size() > 10);
    result.p_value = 1.0 - std::abs(result.correlation); // Simplified p-value
    
    return result;
}

double AdvancedAnalysis::calculateLinearRegression(const std::vector<double>& x, const std::vector<double>& y, double& slope, double& intercept) {
    if (x.size() != y.size() || x.size() < 2) {
        slope = 0.0;
        intercept = 0.0;
        return 0.0;
    }
    
    size_t n = x.size();
    double sum_x = std::accumulate(x.begin(), x.end(), 0.0);
    double sum_y = std::accumulate(y.begin(), y.end(), 0.0);
    double sum_xy = std::inner_product(x.begin(), x.end(), y.begin(), 0.0);
    double sum_x2 = std::inner_product(x.begin(), x.end(), x.begin(), 0.0);
    
    // Calculate slope and intercept
    double denominator = n * sum_x2 - sum_x * sum_x;
    if (std::abs(denominator) < 1e-10) {
        slope = 0.0;
        intercept = sum_y / n;
        return 0.0;
    }
    
    slope = (n * sum_xy - sum_x * sum_y) / denominator;
    intercept = (sum_y - slope * sum_x) / n;
    
    // Calculate R-squared
    double y_mean = sum_y / n;
    double ss_tot = 0.0, ss_res = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double y_pred = slope * x[i] + intercept;
        ss_tot += (y[i] - y_mean) * (y[i] - y_mean);
        ss_res += (y[i] - y_pred) * (y[i] - y_pred);
    }
    
    return (ss_tot > 1e-10) ? (1.0 - ss_res / ss_tot) : 0.0;
}

double AdvancedAnalysis::calculateRSquared(const std::vector<double>& y_actual, const std::vector<double>& y_predicted) {
    if (y_actual.size() != y_predicted.size() || y_actual.empty()) {
        return 0.0;
    }
    
    double y_mean = calculateMean(y_actual);
    double ss_tot = 0.0, ss_res = 0.0;
    
    for (size_t i = 0; i < y_actual.size(); ++i) {
        ss_tot += (y_actual[i] - y_mean) * (y_actual[i] - y_mean);
        ss_res += (y_actual[i] - y_predicted[i]) * (y_actual[i] - y_predicted[i]);
    }
    
    return (ss_tot > 1e-10) ? (1.0 - ss_res / ss_tot) : 0.0;
}

double AdvancedAnalysis::calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 2) {
        return 0.0;
    }
    
    double mean_x = calculateMean(x);
    double mean_y = calculateMean(y);
    double numerator = 0.0, sum_x2 = 0.0, sum_y2 = 0.0;
    
    for (size_t i = 0; i < x.size(); ++i) {
        double dx = x[i] - mean_x;
        double dy = y[i] - mean_y;
        numerator += dx * dy;
        sum_x2 += dx * dx;
        sum_y2 += dy * dy;
    }
    
    double denominator = std::sqrt(sum_x2 * sum_y2);
    return (denominator > 1e-10) ? (numerator / denominator) : 0.0;
}

// Spectral Analysis Implementation
SpectralAnalysis AdvancedAnalysis::performFFT(const std::vector<int64_t>& values, double sampling_rate) {
    SpectralAnalysis result;
    
    if (values.size() < 4) {
        return result; // Empty result for insufficient data
    }
    
    // Convert to double and pad to power of 2
    std::vector<double> input = convertToDouble(values);
    size_t n = 1;
    while (n < input.size()) n <<= 1;
    input.resize(n, 0.0);
    
    // Perform FFT
    std::vector<std::complex<double>> fft_result = fft(input);
    
    // Calculate frequencies and magnitudes
    result.frequencies.resize(n / 2);
    result.magnitudes.resize(n / 2);
    result.phases.resize(n / 2);
    
    double freq_resolution = sampling_rate / n;
    result.total_power = 0.0;
    double max_magnitude = 0.0;
    size_t max_index = 0;
    
    for (size_t i = 0; i < n / 2; ++i) {
        result.frequencies[i] = i * freq_resolution;
        result.magnitudes[i] = std::abs(fft_result[i]);
        result.phases[i] = std::arg(fft_result[i]);
        
        result.total_power += result.magnitudes[i] * result.magnitudes[i];
        
        if (result.magnitudes[i] > max_magnitude) {
            max_magnitude = result.magnitudes[i];
            max_index = i;
        }
    }
    
    result.dominant_frequency = result.frequencies[max_index];
    result.power_bands = analyzeFrequencyBands(result.frequencies, result.magnitudes);
    
    return result;
}

std::vector<std::complex<double>> AdvancedAnalysis::fft(const std::vector<double>& input) {
    size_t n = input.size();
    std::vector<std::complex<double>> result(n);
    
    // Copy input to complex array
    for (size_t i = 0; i < n; ++i) {
        result[i] = std::complex<double>(input[i], 0.0);
    }
    
    // Cooley-Tukey FFT algorithm
    for (size_t len = 1; len < n; len <<= 1) {
        double angle = -M_PI / len;
        std::complex<double> wlen(std::cos(angle), std::sin(angle));
        
        for (size_t i = 0; i < n; i += len << 1) {
            std::complex<double> w(1.0, 0.0);
            for (size_t j = 0; j < len; ++j) {
                std::complex<double> u = result[i + j];
                std::complex<double> v = result[i + j + len] * w;
                result[i + j] = u + v;
                result[i + j + len] = u - v;
                w *= wlen;
            }
        }
    }
    
    return result;
}

std::map<std::string, double> AdvancedAnalysis::analyzeFrequencyBands(const std::vector<double>& frequencies, const std::vector<double>& magnitudes) {
    std::map<std::string, double> bands;
    
    double low_power = 0.0, mid_power = 0.0, high_power = 0.0;
    
    for (size_t i = 0; i < frequencies.size(); ++i) {
        double freq = frequencies[i];
        double power = magnitudes[i] * magnitudes[i];
        
        if (freq < 0.1) {
            low_power += power;
        } else if (freq < 1.0) {
            mid_power += power;
        } else {
            high_power += power;
        }
    }
    
    bands["low_frequency"] = low_power;
    bands["mid_frequency"] = mid_power;
    bands["high_frequency"] = high_power;
    
    return bands;
}

// Anomaly Detection Implementation
AnomalyDetection AdvancedAnalysis::detectAnomalies(const std::vector<int64_t>& values, double threshold_multiplier) {
    AnomalyDetection result;
    
    if (values.size() < 3) {
        return result; // Empty result for insufficient data
    }
    
    // Convert to double for better precision
    std::vector<double> double_values = convertToDouble(values);
    
    // Use IQR method for anomaly detection
    result.outlier_indices = detectOutliersIQR(values, threshold_multiplier);
    result.total_anomalies = result.outlier_indices.size();
    result.anomaly_rate = (double)result.total_anomalies / values.size() * 100.0;
    
    // Calculate outlier scores using modified Z-score
    std::vector<double> z_scores = calculateModifiedZScore(values);
    result.outlier_scores.resize(values.size());
    result.threshold = threshold_multiplier;
    
    for (size_t i = 0; i < values.size(); ++i) {
        result.outlier_scores[i] = std::abs(z_scores[i]);
        if (std::find(result.outlier_indices.begin(), result.outlier_indices.end(), i) != result.outlier_indices.end()) {
            if (z_scores[i] > threshold_multiplier) {
                result.anomaly_types.push_back("high_outlier");
            } else {
                result.anomaly_types.push_back("low_outlier");
            }
        }
    }
    
    return result;
}

std::vector<int> AdvancedAnalysis::detectOutliersIQR(const std::vector<int64_t>& values, double multiplier) {
    std::vector<int> outliers;
    
    if (values.size() < 4) {
        return outliers;
    }
    
    std::vector<int64_t> sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    
    size_t n = sorted_values.size();
    size_t q1_index = n / 4;
    size_t q3_index = 3 * n / 4;
    
    int64_t q1 = sorted_values[q1_index];
    int64_t q3 = sorted_values[q3_index];
    int64_t iqr = q3 - q1;
    
    int64_t lower_bound = q1 - multiplier * iqr;
    int64_t upper_bound = q3 + multiplier * iqr;
    
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] < lower_bound || values[i] > upper_bound) {
            outliers.push_back(i);
        }
    }
    
    return outliers;
}

std::vector<double> AdvancedAnalysis::calculateModifiedZScore(const std::vector<int64_t>& values) {
    std::vector<double> z_scores(values.size());
    
    if (values.size() < 3) {
        return z_scores;
    }
    
    // Calculate median
    std::vector<int64_t> sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    double median = (sorted_values.size() % 2 == 0) ? 
        (sorted_values[sorted_values.size()/2 - 1] + sorted_values[sorted_values.size()/2]) / 2.0 :
        sorted_values[sorted_values.size()/2];
    
    // Calculate median absolute deviation
    std::vector<double> deviations(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        deviations[i] = std::abs(values[i] - median);
    }
    std::sort(deviations.begin(), deviations.end());
    double mad = (deviations.size() % 2 == 0) ?
        (deviations[deviations.size()/2 - 1] + deviations[deviations.size()/2]) / 2.0 :
        deviations[deviations.size()/2];
    
    // Calculate modified Z-scores
    double mad_scaled = mad * 1.4826; // Scale factor for normal distribution
    for (size_t i = 0; i < values.size(); ++i) {
        z_scores[i] = (mad_scaled > 1e-10) ? (values[i] - median) / mad_scaled : 0.0;
    }
    
    return z_scores;
}

// Utility Functions
std::vector<double> AdvancedAnalysis::convertToDouble(const std::vector<int64_t>& values) {
    std::vector<double> result(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        result[i] = static_cast<double>(values[i]);
    }
    return result;
}

std::vector<int64_t> AdvancedAnalysis::generateTimestamps(int64_t start_time, int64_t interval, size_t count) {
    std::vector<int64_t> timestamps(count);
    for (size_t i = 0; i < count; ++i) {
        timestamps[i] = start_time + i * interval;
    }
    return timestamps;
}

double AdvancedAnalysis::calculateMean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double AdvancedAnalysis::calculateStdDev(const std::vector<double>& values, double mean) {
    if (values.size() < 2) return 0.0;
    
    double sum_squared_diff = 0.0;
    for (double value : values) {
        double diff = value - mean;
        sum_squared_diff += diff * diff;
    }
    
    return std::sqrt(sum_squared_diff / (values.size() - 1));
}

std::string AdvancedAnalysis::formatDuration(double milliseconds) {
    std::ostringstream oss;
    if (milliseconds < 1000) {
        oss << std::fixed << std::setprecision(1) << milliseconds << " мс";
    } else if (milliseconds < 60000) {
        oss << std::fixed << std::setprecision(1) << (milliseconds / 1000) << " сек";
    } else {
        oss << std::fixed << std::setprecision(1) << (milliseconds / 60000) << " мин";
    }
    return oss.str();
}

std::string AdvancedAnalysis::formatFrequency(double frequency) {
    std::ostringstream oss;
    if (frequency < 0.001) {
        oss << std::scientific << std::setprecision(2) << frequency << " Гц";
    } else if (frequency < 1.0) {
        oss << std::fixed << std::setprecision(3) << frequency << " Гц";
    } else {
        oss << std::fixed << std::setprecision(1) << frequency << " Гц";
    }
    return oss.str();
}

// Comprehensive Analysis
AdvancedStatistics AdvancedAnalysis::performComprehensiveAnalysis(const PHCResult& result) {
    AdvancedStatistics stats;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    if (!result.success || result.differences.empty()) {
        return stats; // Return empty statistics
    }
    
    // Extract time series data for analysis
    std::vector<int64_t> time_series;
    for (const auto& measurement : result.differences) {
        if (!measurement.empty()) {
            time_series.push_back(measurement[0]); // Use first device pair
        }
    }
    
    if (time_series.size() < 2) {
        return stats; // Insufficient data
    }
    
    // Convert absolute PTP times to relative differences for analysis
    // This removes the epoch offset and focuses on the actual time differences
    std::vector<int64_t> relative_differences;
    if (time_series.size() > 1) {
        int64_t base_time = time_series[0];
        for (int64_t value : time_series) {
            relative_differences.push_back(value - base_time);
        }
    } else {
        relative_differences = time_series;
    }
    
    // Perform trend analysis on relative differences
    stats.trend = analyzeTrend(relative_differences);
    
    // Perform spectral analysis on relative differences
    stats.spectral = performFFT(relative_differences, 1.0); // 1 Hz sampling rate
    
    // Perform anomaly detection on relative differences
    stats.anomalies = detectAnomalies(relative_differences);
    
    // Set metadata
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats.analysis_duration_ms = duration.count() / 1000.0;
    stats.data_points_analyzed = time_series.size();
    
    return stats;
}
