#ifndef SHIWADIFFPHC_GUI_H
#define SHIWADIFFPHC_GUI_H

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QGroupBox>
#include <QComboBox>
#include <QProgressBar>
#include <QTimer>
#include <QStatusBar>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QHeaderView>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QScatterSeries>
#include <QAreaSeries>
#include <vector>

#include "diffphc_core.h"

QT_CHARTS_USE_NAMESPACE

class ShiwaDiffPHCMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ShiwaDiffPHCMainWindow(QWidget *parent = nullptr);
    ~ShiwaDiffPHCMainWindow();

private slots:
    void onStartMeasurement();
    void onStopMeasurement();
    void onDeviceSelectionChanged();
    void onConfigChanged();
    void onTimerUpdate();
    void onSaveResults();
    void onLoadConfig();
    void onSaveConfig();
    void onShowDeviceInfo();
    void onRefreshDevices();
    void onAbout();

private:
    void setupUI();
    void setupMenuBar();
    void setupControlPanel();
    void setupResultsPanel();
    void setupStatusBar();
    void updateDeviceList();
    void updateResultsTable(const PHCResult& result);
    void updateStatisticsTable(const PHCResult& result);
    void updatePlot(const PHCResult& result);
    void logMessage(const QString& message);
    void clearResults();
    bool validateConfiguration();
    PHCConfig getCurrentConfig();

    // UI Components
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    QTabWidget* m_tabWidget;
    
    // Control Panel
    QGroupBox* m_deviceGroup;
    QGroupBox* m_configGroup;
    QGroupBox* m_controlGroup;
    
    QComboBox* m_deviceCombo;
    QCheckBox* m_deviceCheckBoxes[8]; // Support up to 8 PTP devices
    QSpinBox* m_countSpinBox;
    QSpinBox* m_delaySpinBox;
    QSpinBox* m_samplesSpinBox;
    QCheckBox* m_continuousCheckBox;
    QCheckBox* m_verboseCheckBox;
    
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QPushButton* m_saveButton;
    QPushButton* m_clearButton;
    QPushButton* m_refreshButton;
    QPushButton* m_infoButton;
    
    // Results Panel
    QTableWidget* m_resultsTable;
    QTableWidget* m_statisticsTable;
    QChartView* m_plotWidget; // Real chart widget
    QPlainTextEdit* m_logTextEdit;
    
    // Status Bar
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QLabel* m_deviceCountLabel;
    
    // Measurement state
    QTimer* m_measurementTimer;
    PHCConfig m_currentConfig;
    std::vector<PHCResult> m_results;
    bool m_measuring;
    int m_currentIteration;
    std::vector<int> m_availableDevices;
};

#endif // SHIWADIFFPHC_GUI_H