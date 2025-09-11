#ifndef ADVANCED_ANALYSIS_H
#define ADVANCED_ANALYSIS_H

#include <vector>
#include <string>
#include <map>
#include <complex>

// Forward declarations
struct PHCResult;
struct PHCStatistics;

// Trend Analysis Structures
struct TrendAnalysis {
    double slope;           // Наклон тренда (нс/сек)
    double intercept;       // Пересечение с осью Y
    double r_squared;       // Коэффициент детерминации R²
    double correlation;     // Коэффициент корреляции
    double p_value;         // P-значение для статистической значимости
    std::string trend_type; // "increasing", "decreasing", "stable"
    bool is_significant;    // Статистически значимый тренд
};

// Spectral Analysis Structures
struct SpectralAnalysis {
    std::vector<double> frequencies;     // Частоты (Гц)
    std::vector<double> magnitudes;      // Амплитуды
    std::vector<double> phases;          // Фазы
    double dominant_frequency;           // Доминирующая частота
    double total_power;                  // Общая мощность
    std::map<std::string, double> power_bands; // Мощность в частотных полосах
};

// Correlation Analysis Structures
struct CorrelationAnalysis {
    std::map<std::string, double> correlations; // Корреляции между парами устройств
    double max_correlation;                     // Максимальная корреляция
    double min_correlation;                     // Минимальная корреляция
    std::string strongest_pair;                 // Наиболее коррелированная пара
    std::string weakest_pair;                   // Наименее коррелированная пара
};

// Anomaly Detection Structures
struct AnomalyDetection {
    std::vector<int> outlier_indices;           // Индексы выбросов
    std::vector<double> outlier_scores;         // Оценки аномальности
    double threshold;                           // Порог для детекции
    int total_anomalies;                        // Общее количество аномалий
    double anomaly_rate;                        // Процент аномалий
    std::vector<std::string> anomaly_types;     // Типы аномалий
};

// Time Series Prediction Structures
struct TimeSeriesPrediction {
    std::vector<double> predicted_values;       // Предсказанные значения
    std::vector<double> confidence_intervals;   // Доверительные интервалы
    double prediction_accuracy;                 // Точность предсказания
    int prediction_horizon;                     // Горизонт предсказания
    std::string model_type;                     // Тип модели
};

// Advanced Statistics Container
struct AdvancedStatistics {
    TrendAnalysis trend;
    SpectralAnalysis spectral;
    CorrelationAnalysis correlation;
    AnomalyDetection anomalies;
    TimeSeriesPrediction prediction;
    
    // Metadata
    std::string analysis_timestamp;
    int data_points_analyzed;
    double analysis_duration_ms;
};

class AdvancedAnalysis {
public:
    // Trend Analysis
    static TrendAnalysis analyzeTrend(const std::vector<int64_t>& values, const std::vector<int64_t>& timestamps = {});
    static double calculateLinearRegression(const std::vector<double>& x, const std::vector<double>& y, double& slope, double& intercept);
    static double calculateRSquared(const std::vector<double>& y_actual, const std::vector<double>& y_predicted);
    static double calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y);
    
    // Spectral Analysis
    static SpectralAnalysis performFFT(const std::vector<int64_t>& values, double sampling_rate = 1.0);
    static std::vector<std::complex<double>> fft(const std::vector<double>& input);
    static std::vector<double> calculatePowerSpectralDensity(const std::vector<std::complex<double>>& fft_result);
    static std::map<std::string, double> analyzeFrequencyBands(const std::vector<double>& frequencies, const std::vector<double>& magnitudes);
    
    // Correlation Analysis
    static CorrelationAnalysis analyzeCorrelations(const PHCResult& result);
    static double calculatePearsonCorrelation(const std::vector<double>& x, const std::vector<double>& y);
    static std::map<std::string, double> calculatePairwiseCorrelations(const std::vector<std::vector<int64_t>>& measurements);
    
    // Anomaly Detection
    static AnomalyDetection detectAnomalies(const std::vector<int64_t>& values, double threshold_multiplier = 2.0);
    static std::vector<double> calculateZScore(const std::vector<int64_t>& values);
    static std::vector<double> calculateModifiedZScore(const std::vector<int64_t>& values);
    static std::vector<int> detectOutliersIQR(const std::vector<int64_t>& values, double multiplier = 1.5);
    static std::vector<int> detectOutliersZScore(const std::vector<int64_t>& values, double threshold = 3.0);
    
    // Time Series Prediction
    static TimeSeriesPrediction predictTimeSeries(const std::vector<int64_t>& values, int horizon = 10);
    static std::vector<double> simpleMovingAverage(const std::vector<int64_t>& values, int window_size = 5);
    static std::vector<double> exponentialSmoothing(const std::vector<int64_t>& values, double alpha = 0.3);
    static std::vector<double> linearTrendPrediction(const std::vector<int64_t>& values, int horizon);
    
    // Comprehensive Analysis
    static AdvancedStatistics performComprehensiveAnalysis(const PHCResult& result);
    
    // Utility Functions
    static std::vector<double> convertToDouble(const std::vector<int64_t>& values);
    static std::vector<int64_t> generateTimestamps(int64_t start_time, int64_t interval, size_t count);
    static double calculateMean(const std::vector<double>& values);
    static double calculateStdDev(const std::vector<double>& values, double mean);
    static std::string formatDuration(double milliseconds);
    static std::string formatFrequency(double frequency);
};

#endif // ADVANCED_ANALYSIS_H
