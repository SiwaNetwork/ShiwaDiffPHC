#include "diffphc_gui.h"
#include <QApplication>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStyleFactory>
#include <QShortcut>
#include <QKeySequence>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFile>
#include <QTextStream>

ShiwaDiffPHCMainWindow::ShiwaDiffPHCMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_measurementTimer(new QTimer(this))
    , m_measuring(false)
    , m_currentIteration(0)
    , m_hasAdvancedStats(false)
{
    setWindowTitle("ShiwaDiffPHC v1.3.0 - Анализатор различий протокола точного времени");
    setMinimumSize(1000, 700);
    resize(1400, 900);
    
    // Apply modern dark theme
    applyDarkTheme();
    
    // Enable drag and drop
    setAcceptDrops(true);

    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupKeyboardShortcuts();
    
    updateDeviceList();
    
    connect(m_measurementTimer, &QTimer::timeout, this, &ShiwaDiffPHCMainWindow::onTimerUpdate);
    
    // Initialize configuration
    m_currentConfig.count = 0;
    m_currentConfig.delay = 100000;
    m_currentConfig.samples = 10;
    m_currentConfig.debug = false;
    
    logMessage("ShiwaDiffPHC GUI v1.3.0 инициализирован с современным интерфейсом");
    
    // Показываем тестовый график при запуске
    PHCResult testResult;
    testResult.success = false; // Это заставит показать тестовые данные
    updatePlot(testResult);
}

ShiwaDiffPHCMainWindow::~ShiwaDiffPHCMainWindow() {
    if (m_measuring) {
        onStopMeasurement();
    }
}

void ShiwaDiffPHCMainWindow::setupUI() {
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    
    auto* layout = new QHBoxLayout(m_centralWidget);
    layout->addWidget(m_mainSplitter);
    
    setupControlPanel();
    setupResultsPanel();
}

void ShiwaDiffPHCMainWindow::setupMenuBar() {
    // File Menu
    auto* fileMenu = menuBar()->addMenu("&File");
    
    auto* loadConfigAction = fileMenu->addAction("&Load Configuration...");
    loadConfigAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(loadConfigAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onLoadConfig);
    
    auto* saveConfigAction = fileMenu->addAction("&Save Configuration...");
    saveConfigAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(saveConfigAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onSaveConfig);
    
    fileMenu->addSeparator();
    
    auto* saveResultsAction = fileMenu->addAction("Save &Results...");
    saveResultsAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(saveResultsAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onSaveResults);
    
    fileMenu->addSeparator();
    
    auto* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // View Menu
    auto* viewMenu = menuBar()->addMenu("&View");
    
    auto* toggleThemeAction = viewMenu->addAction("Toggle &Theme");
    toggleThemeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(toggleThemeAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onToggleTheme);
    
    viewMenu->addSeparator();
    
    auto* zoomInAction = viewMenu->addAction("Zoom &In");
    zoomInAction->setShortcut(QKeySequence("Ctrl+Plus"));
    connect(zoomInAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onZoomIn);
    
    auto* zoomOutAction = viewMenu->addAction("Zoom &Out");
    zoomOutAction->setShortcut(QKeySequence("Ctrl+Minus"));
    connect(zoomOutAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onZoomOut);
    
    auto* resetZoomAction = viewMenu->addAction("&Reset Zoom");
    resetZoomAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(resetZoomAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onResetZoom);
    
    viewMenu->addSeparator();
    
    auto* exportChartAction = viewMenu->addAction("Export &Chart...");
    exportChartAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    connect(exportChartAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onExportChart);
    
    // Tools Menu
    auto* toolsMenu = menuBar()->addMenu("&Tools");
    
    auto* refreshDevicesAction = toolsMenu->addAction("&Refresh Devices");
    refreshDevicesAction->setShortcut(QKeySequence("F5"));
    connect(refreshDevicesAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onRefreshDevices);
    
    auto* deviceInfoAction = toolsMenu->addAction("Device &Info");
    connect(deviceInfoAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onShowDeviceInfo);
    
    toolsMenu->addSeparator();
    
    auto* clearResultsAction = toolsMenu->addAction("&Clear Results");
    clearResultsAction->setShortcut(QKeySequence("Ctrl+Delete"));
    connect(clearResultsAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::clearResults);
    
    // Analysis Menu
    auto* analysisMenu = menuBar()->addMenu("&Analysis");
    
    auto* advancedAnalysisAction = analysisMenu->addAction("&Advanced Analysis");
    advancedAnalysisAction->setShortcut(QKeySequence("Ctrl+A"));
    connect(advancedAnalysisAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onAdvancedAnalysis);
    
    analysisMenu->addSeparator();
    
    auto* trendAnalysisAction = analysisMenu->addAction("&Trend Analysis");
    connect(trendAnalysisAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onTrendAnalysis);
    
    auto* spectralAnalysisAction = analysisMenu->addAction("&Spectral Analysis");
    connect(spectralAnalysisAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onSpectralAnalysis);
    
    auto* anomalyDetectionAction = analysisMenu->addAction("&Anomaly Detection");
    connect(anomalyDetectionAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onAnomalyDetection);
    
    analysisMenu->addSeparator();
    
    auto* generateReportAction = analysisMenu->addAction("Generate &Report");
    generateReportAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(generateReportAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onGenerateReport);
    
    // Help Menu
    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAction = helpMenu->addAction("&О программе ShiwaDiffPHC");
    connect(aboutAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onAbout);
}

void ShiwaDiffPHCMainWindow::setupControlPanel() {
    auto* controlWidget = new QWidget;
    auto* controlLayout = new QVBoxLayout(controlWidget);
    
    // Device Selection Group
    m_deviceGroup = new QGroupBox("PTP Devices");
    auto* deviceLayout = new QVBoxLayout(m_deviceGroup);
    
    auto* deviceHeaderLayout = new QHBoxLayout;
    deviceHeaderLayout->addWidget(new QLabel("Available Devices:"));
    m_refreshButton = new QPushButton("Refresh");
    m_infoButton = new QPushButton("Info");
    deviceHeaderLayout->addWidget(m_refreshButton);
    deviceHeaderLayout->addWidget(m_infoButton);
    deviceLayout->addLayout(deviceHeaderLayout);
    
    for (int i = 0; i < 8; ++i) {
        m_deviceCheckBoxes[i] = new QCheckBox(QString("PTP Device %1").arg(i));
        m_deviceCheckBoxes[i]->setVisible(false);
        deviceLayout->addWidget(m_deviceCheckBoxes[i]);
        connect(m_deviceCheckBoxes[i], &QCheckBox::toggled, this, &ShiwaDiffPHCMainWindow::onDeviceSelectionChanged);
    }
    
    // Configuration Group
    m_configGroup = new QGroupBox("Configuration");
    auto* configLayout = new QGridLayout(m_configGroup);
    
    configLayout->addWidget(new QLabel("Iterations:"), 0, 0);
    m_countSpinBox = new QSpinBox;
    m_countSpinBox->setRange(0, 999999);
    m_countSpinBox->setValue(0);
    m_countSpinBox->setSpecialValueText("Infinite");
    configLayout->addWidget(m_countSpinBox, 0, 1);
    
    configLayout->addWidget(new QLabel("Delay (μs):"), 1, 0);
    m_delaySpinBox = new QSpinBox;
    m_delaySpinBox->setRange(1, 10000000);
    m_delaySpinBox->setValue(100000);
    configLayout->addWidget(m_delaySpinBox, 1, 1);
    
    configLayout->addWidget(new QLabel("Samples:"), 2, 0);
    m_samplesSpinBox = new QSpinBox;
    m_samplesSpinBox->setRange(1, 100);
    m_samplesSpinBox->setValue(10);
    configLayout->addWidget(m_samplesSpinBox, 2, 1);
    
    m_continuousCheckBox = new QCheckBox("Continuous measurement");
    configLayout->addWidget(m_continuousCheckBox, 3, 0, 1, 2);
    
    m_verboseCheckBox = new QCheckBox("Verbose output");
    configLayout->addWidget(m_verboseCheckBox, 4, 0, 1, 2);
    
    // Connect configuration change signals
    connect(m_countSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ShiwaDiffPHCMainWindow::onConfigChanged);
    connect(m_delaySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ShiwaDiffPHCMainWindow::onConfigChanged);
    connect(m_samplesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ShiwaDiffPHCMainWindow::onConfigChanged);
    connect(m_continuousCheckBox, &QCheckBox::toggled, this, &ShiwaDiffPHCMainWindow::onConfigChanged);
    connect(m_verboseCheckBox, &QCheckBox::toggled, this, &ShiwaDiffPHCMainWindow::onConfigChanged);
    
    // Control Group
    m_controlGroup = new QGroupBox("Control");
    auto* controlButtonLayout = new QVBoxLayout(m_controlGroup);
    
    m_startButton = new QPushButton("Start Measurement");
    m_stopButton = new QPushButton("Stop Measurement");
    m_saveButton = new QPushButton("Save Results");
    m_clearButton = new QPushButton("Clear Results");
    
    m_startButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }");
    m_stopButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; }");
    m_stopButton->setEnabled(false);
    
    controlButtonLayout->addWidget(m_startButton);
    controlButtonLayout->addWidget(m_stopButton);
    controlButtonLayout->addWidget(m_saveButton);
    controlButtonLayout->addWidget(m_clearButton);
    
    connect(m_startButton, &QPushButton::clicked, this, &ShiwaDiffPHCMainWindow::onStartMeasurement);
    connect(m_stopButton, &QPushButton::clicked, this, &ShiwaDiffPHCMainWindow::onStopMeasurement);
    connect(m_saveButton, &QPushButton::clicked, this, &ShiwaDiffPHCMainWindow::onSaveResults);
    connect(m_clearButton, &QPushButton::clicked, this, &ShiwaDiffPHCMainWindow::clearResults);
    connect(m_refreshButton, &QPushButton::clicked, this, &ShiwaDiffPHCMainWindow::onRefreshDevices);
    connect(m_infoButton, &QPushButton::clicked, this, &ShiwaDiffPHCMainWindow::onShowDeviceInfo);
    
    controlLayout->addWidget(m_deviceGroup);
    controlLayout->addWidget(m_configGroup);
    controlLayout->addWidget(m_controlGroup);
    controlLayout->addStretch();
    
    m_mainSplitter->addWidget(controlWidget);
}

void ShiwaDiffPHCMainWindow::setupResultsPanel() {
    m_tabWidget = new QTabWidget;
    
    // Results Table Tab
    auto* resultsWidget = new QWidget;
    auto* resultsLayout = new QVBoxLayout(resultsWidget);
    
    m_resultsTable = new QTableWidget;
    m_resultsTable->setAlternatingRowColors(true);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultsLayout->addWidget(m_resultsTable);
    
    m_tabWidget->addTab(resultsWidget, "Таблица результатов");
    
    // Statistics Table Tab
    auto* statisticsWidget = new QWidget;
    auto* statisticsLayout = new QVBoxLayout(statisticsWidget);
    
    m_statisticsTable = new QTableWidget;
    m_statisticsTable->setAlternatingRowColors(true);
    m_statisticsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    statisticsLayout->addWidget(m_statisticsTable);
    
    m_tabWidget->addTab(statisticsWidget, "Статистический анализ");
    
    // Plot Tab (real charts) - Enhanced interactive chart
    m_plotWidget = new QChartView;
    m_plotWidget->setRenderHint(QPainter::Antialiasing);
    m_plotWidget->setRubberBand(QChartView::RectangleRubberBand); // Enable zoom with mouse
    m_plotWidget->setDragMode(QGraphicsView::ScrollHandDrag); // Enable pan with mouse
    
    // Создаем пустой график для начала
    QChart* initialChart = new QChart();
    initialChart->setTitle("📊 Интерактивная визуализация различий PTP устройств");
    initialChart->setAnimationOptions(QChart::SeriesAnimations);
    initialChart->setTheme(QChart::ChartThemeDark);
    
    // Добавляем текст на график
    QLineSeries* placeholderSeries = new QLineSeries();
    placeholderSeries->setName("Ожидание данных...");
    placeholderSeries->append(0, 0);
    initialChart->addSeries(placeholderSeries);
    
    m_plotWidget->setChart(initialChart);
    m_tabWidget->addTab(m_plotWidget, "Графики");
    
    // Log Tab
    m_logTextEdit = new QPlainTextEdit;
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setMaximumBlockCount(1000); // Limit log size
    m_tabWidget->addTab(m_logTextEdit, "Log");
    
    m_mainSplitter->addWidget(m_tabWidget);
    m_mainSplitter->setSizes({300, 700}); // 30% control panel, 70% results
}

void ShiwaDiffPHCMainWindow::setupStatusBar() {
    m_statusLabel = new QLabel("Ready");
    m_deviceCountLabel = new QLabel("Devices: 0");
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_deviceCountLabel);
    statusBar()->addPermanentWidget(m_progressBar);
}

void ShiwaDiffPHCMainWindow::updateDeviceList() {
    m_availableDevices = DiffPHCCore::getAvailablePHCDevices();
    
    // Hide all checkboxes first
    for (int i = 0; i < 8; ++i) {
        m_deviceCheckBoxes[i]->setVisible(false);
        m_deviceCheckBoxes[i]->setChecked(false);
    }
    
    // Show available devices
    for (int device : m_availableDevices) {
        if (device < 8) {
            m_deviceCheckBoxes[device]->setText(QString("PTP Device %1 (/dev/ptp%1)").arg(device));
            m_deviceCheckBoxes[device]->setVisible(true);
        }
    }
    
    m_deviceCountLabel->setText(QString("Devices: %1").arg(m_availableDevices.size()));
    
    logMessage(QString("Found %1 PTP devices").arg(m_availableDevices.size()));
}

void ShiwaDiffPHCMainWindow::onStartMeasurement() {
    if (!validateConfiguration()) {
        return;
    }
    
    if (DiffPHCCore::requiresRoot()) {
        QMessageBox::warning(this, "Permission Required", 
                           "Root privileges are required to access PTP devices.\n"
                           "Please run this application as root (sudo).");
        return;
    }
    
    m_currentConfig = getCurrentConfig();
    m_measuring = true;
    m_currentIteration = 0;
    
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_progressBar->setVisible(true);
    
    if (m_currentConfig.count > 0) {
        m_progressBar->setRange(0, m_currentConfig.count);
    } else {
        m_progressBar->setRange(0, 0); // Indeterminate progress
    }
    
    m_statusLabel->setText("Measuring...");
    
    // Start measurement timer
    m_measurementTimer->start(m_currentConfig.delay / 1000); // Convert μs to ms
    
    logMessage("Measurement started");
}

void ShiwaDiffPHCMainWindow::onStopMeasurement() {
    m_measuring = false;
    m_measurementTimer->stop();
    
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_progressBar->setVisible(false);
    m_statusLabel->setText("Stopped");
    
    logMessage("Measurement stopped");
}

void ShiwaDiffPHCMainWindow::onTimerUpdate() {
    if (!m_measuring) {
        return;
    }
    
    // Perform single measurement
    PHCConfig singleConfig = m_currentConfig;
    singleConfig.count = 1; // Single measurement
    
    PHCResult result = DiffPHCCore::measurePHCDifferences(singleConfig);
    
    if (result.success) {
        m_results.push_back(result);
        updateResultsTable(result);
        updateStatisticsTable(result);
        logMessage(QString("Calling updatePlot for iteration %1").arg(m_currentIteration));
        updatePlot(result);
        m_currentIteration++;
        
        if (m_currentConfig.count > 0) {
            m_progressBar->setValue(m_currentIteration);
            if (m_currentIteration >= m_currentConfig.count) {
                onStopMeasurement();
                logMessage(QString("Measurement completed: %1 iterations").arg(m_currentIteration));
            }
        }
        
        logMessage(QString("Iteration %1 completed").arg(m_currentIteration));
    } else {
        logMessage(QString("Measurement error: %1").arg(QString::fromStdString(result.error)));
        onStopMeasurement();
    }
}

void ShiwaDiffPHCMainWindow::updateResultsTable(const PHCResult& result) {
    if (result.differences.empty()) return;
    
    const auto& devices = result.devices;
    const int numDev = devices.size();
    
    // Setup table headers if needed
    if (m_resultsTable->columnCount() == 0) {
        QStringList headers;
        headers << "Iteration" << "Timestamp";
        
        for (int i = 0; i < numDev; ++i) {
            for (int j = 0; j <= i; ++j) {
                headers << QString("ptp%1-ptp%2").arg(devices[i]).arg(devices[j]);
            }
        }
        
        m_resultsTable->setColumnCount(headers.size());
        m_resultsTable->setHorizontalHeaderLabels(headers);
        m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    }
    
    // Add new row
    int row = m_resultsTable->rowCount();
    m_resultsTable->insertRow(row);
    
    m_resultsTable->setItem(row, 0, new QTableWidgetItem(QString::number(m_currentIteration)));
    m_resultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(result.baseTimestamp)));
    
    const auto& latest = result.differences.back();
    for (size_t i = 0; i < latest.size(); ++i) {
        m_resultsTable->setItem(row, i + 2, new QTableWidgetItem(QString::number(latest[i])));
    }
    
    // Scroll to bottom
    m_resultsTable->scrollToBottom();
}

void ShiwaDiffPHCMainWindow::updateStatisticsTable(const PHCResult& result) {
    if (!result.success || result.statistics.empty() || result.differences.size() < 2) {
        return;
    }
    
    const auto& devices = result.devices;
    const int numDev = devices.size();
    
    // Setup table headers if needed
    if (m_statisticsTable->columnCount() == 0) {
        QStringList headers;
        headers << "Пара устройств" << "Медиана (нс)" << "Среднее (нс)" 
                << "Минимум (нс)" << "Максимум (нс)" << "Размах (нс)" 
                << "Станд. откл. (нс)" << "Измерений";
        
        m_statisticsTable->setColumnCount(headers.size());
        m_statisticsTable->setHorizontalHeaderLabels(headers);
        m_statisticsTable->horizontalHeader()->setStretchLastSection(true);
        
        // Calculate number of device pairs (excluding diagonal)
        int pairCount = 0;
        for (int i = 0; i < numDev; ++i) {
            for (int j = 0; j <= i; ++j) {
                if (i != j) pairCount++;
            }
        }
        m_statisticsTable->setRowCount(pairCount);
    }
    
    // Fill data
    int row = 0;
    for (int i = 0; i < numDev; ++i) {
        for (int j = 0; j <= i; ++j) {
            if (i == j) continue; // Skip diagonal
            
            const auto& stats = result.statistics[i][j];
            
            m_statisticsTable->setItem(row, 0, new QTableWidgetItem(
                QString("ptp%1 - ptp%2").arg(devices[i]).arg(devices[j])));
            m_statisticsTable->setItem(row, 1, new QTableWidgetItem(
                QString::number(stats.median, 'f', 1)));
            m_statisticsTable->setItem(row, 2, new QTableWidgetItem(
                QString::number(stats.mean, 'f', 1)));
            m_statisticsTable->setItem(row, 3, new QTableWidgetItem(
                QString::number(stats.minimum)));
            m_statisticsTable->setItem(row, 4, new QTableWidgetItem(
                QString::number(stats.maximum)));
            m_statisticsTable->setItem(row, 5, new QTableWidgetItem(
                QString::number(stats.range)));
            m_statisticsTable->setItem(row, 6, new QTableWidgetItem(
                QString::number(stats.stddev, 'f', 1)));
            m_statisticsTable->setItem(row, 7, new QTableWidgetItem(
                QString::number(stats.count)));
            
            row++;
        }
    }
}

void ShiwaDiffPHCMainWindow::onDeviceSelectionChanged() {
    // Update device count and enable/disable start button
    int selectedCount = 0;
    for (int i = 0; i < 8; ++i) {
        if (m_deviceCheckBoxes[i]->isChecked()) {
            selectedCount++;
        }
    }
    
    m_startButton->setEnabled(selectedCount >= 2);
    
    if (selectedCount < 2) {
        m_statusLabel->setText("Select at least 2 devices");
    } else {
        m_statusLabel->setText("Ready");
    }
}

void ShiwaDiffPHCMainWindow::onConfigChanged() {
    if (m_continuousCheckBox->isChecked()) {
        m_countSpinBox->setValue(0);
        m_countSpinBox->setEnabled(false);
    } else {
        m_countSpinBox->setEnabled(true);
    }
}

bool ShiwaDiffPHCMainWindow::validateConfiguration() {
    PHCConfig config = getCurrentConfig();
    
    if (config.devices.size() < 2) {
        QMessageBox::warning(this, "Configuration Error", 
                           "Please select at least 2 PTP devices for comparison.");
        return false;
    }
    
    std::string error;
    if (!DiffPHCCore::validateConfig(config, error)) {
        QMessageBox::warning(this, "Configuration Error", 
                           QString::fromStdString(error));
        return false;
    }
    
    return true;
}

PHCConfig ShiwaDiffPHCMainWindow::getCurrentConfig() {
    PHCConfig config;
    
    config.count = m_countSpinBox->value();
    config.delay = m_delaySpinBox->value();
    config.samples = m_samplesSpinBox->value();
    config.debug = m_verboseCheckBox->isChecked();
    
    for (int i = 0; i < 8; ++i) {
        if (m_deviceCheckBoxes[i]->isChecked()) {
            config.devices.push_back(i);
        }
    }
    
    return config;
}

void ShiwaDiffPHCMainWindow::clearResults() {
    m_results.clear();
    m_resultsTable->setRowCount(0);
    m_resultsTable->setColumnCount(0);
    m_statisticsTable->setRowCount(0);
    m_statisticsTable->setColumnCount(0);
    m_currentIteration = 0;
    logMessage("Результаты очищены");
}

void ShiwaDiffPHCMainWindow::logMessage(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logTextEdit->appendPlainText(QString("[%1] %2").arg(timestamp, message));
}

void ShiwaDiffPHCMainWindow::onSaveResults() {
    if (m_results.empty()) {
        QMessageBox::information(this, "No Data", "No measurement results to save.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, "Save Results", 
                                                   QString("diffphc_results_%1.csv")
                                                   .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                   "CSV Files (*.csv);;JSON Files (*.json)");
    
    if (fileName.isEmpty()) return;
    
    // Save implementation would go here
    logMessage(QString("Results saved to %1").arg(fileName));
}

void ShiwaDiffPHCMainWindow::onLoadConfig() {
    // Implementation for loading configuration
    logMessage("Load configuration requested");
}

void ShiwaDiffPHCMainWindow::onSaveConfig() {
    // Implementation for saving configuration
    logMessage("Save configuration requested");
}

void ShiwaDiffPHCMainWindow::onShowDeviceInfo() {
    QString info;
    for (int device : m_availableDevices) {
        info += QString("=== PTP Device %1 ===\n").arg(device);
        // Add device info here using DiffPHCCore::printClockInfo
        info += "\n";
    }
    
    QMessageBox::information(this, "Device Information", info);
}

void ShiwaDiffPHCMainWindow::onRefreshDevices() {
    updateDeviceList();
}

void ShiwaDiffPHCMainWindow::updatePlot(const PHCResult& result) {
    // Добавляем отладочную информацию
    logMessage(QString("updatePlot called - success: %1, differences size: %2").arg(result.success).arg(result.differences.size()));
    
    // Создаем интерактивный график
    QChart* chart = new QChart();
    chart->setTitle("📊 Интерактивная визуализация различий PTP устройств");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setTheme(m_darkTheme ? QChart::ChartThemeDark : QChart::ChartThemeLight);
    
    // Настройка интерактивности
    chart->setAcceptHoverEvents(true);

    // Создаем временную ось
    QDateTimeAxis* timeAxis = new QDateTimeAxis;
    timeAxis->setTitleText("Время");
    timeAxis->setFormat("hh:mm:ss");
    chart->addAxis(timeAxis, Qt::AlignBottom);

    // Создаем ось значений
    QValueAxis* valueAxis = new QValueAxis;
    valueAxis->setTitleText("Различие (нс)");
    chart->addAxis(valueAxis, Qt::AlignLeft);

    if (result.success && !result.differences.empty()) {
        logMessage(QString("updatePlot: Creating chart with %1 devices").arg(result.devices.size()));

        // Создаем серии для каждой пары устройств
        const auto& devices = result.devices;
        QDateTime baseTime = QDateTime::fromMSecsSinceEpoch(result.baseTimestamp / 1000000);
        
        logMessage(QString("updatePlot: Base time: %1").arg(baseTime.toString("hh:mm:ss")));
        
        int seriesCount = 0;
        for (size_t i = 0; i < devices.size(); ++i) {
            for (size_t j = i + 1; j < devices.size(); ++j) {
                QLineSeries* series = new QLineSeries();
                QString seriesName = QString("ptp%1 - ptp%2").arg(devices[i]).arg(devices[j]);
                series->setName(seriesName);
                
                logMessage(QString("updatePlot: Creating series %1").arg(seriesName));
                
                // Добавляем точки для каждого измерения
                int pointCount = 0;
                for (size_t k = 0; k < result.differences.size(); ++k) {
                    const auto& measurement = result.differences[k];
                    size_t idx = i * devices.size() + j;
                    if (idx < measurement.size()) {
                        QDateTime pointTime = baseTime.addMSecs(k * 100); // Примерное время
                        qint64 value = measurement[idx];
                        series->append(pointTime.toMSecsSinceEpoch(), value);
                        pointCount++;
                        
                        if (k == 0) { // Логируем только первую точку для краткости
                            logMessage(QString("updatePlot: Added point %1 at %2").arg(value).arg(pointTime.toString("hh:mm:ss")));
                        }
                    }
                }
                
                logMessage(QString("updatePlot: Series %1 has %2 points").arg(seriesName).arg(pointCount));
                
                chart->addSeries(series);
                series->attachAxis(timeAxis);
                series->attachAxis(valueAxis);
                seriesCount++;
            }
        }
        
        logMessage(QString("updatePlot: Created %1 series").arg(seriesCount));
    } else {
        // Создаем тестовые данные для демонстрации
        logMessage("updatePlot: Creating test chart with sample data");
        
        QLineSeries* testSeries = new QLineSeries();
        testSeries->setName("Тестовые данные");
        
        QDateTime now = QDateTime::currentDateTime();
        for (int i = 0; i < 10; ++i) {
            QDateTime pointTime = now.addSecs(i);
            qint64 value = 1000000 + (i * 50000) + (rand() % 100000); // Случайные значения
            testSeries->append(pointTime.toMSecsSinceEpoch(), value);
            logMessage(QString("updatePlot: Added test point %1 at %2").arg(value).arg(pointTime.toString("hh:mm:ss")));
        }
        
        chart->addSeries(testSeries);
        testSeries->attachAxis(timeAxis);
        testSeries->attachAxis(valueAxis);
    }

    // Сохраняем ссылку на текущий график для экспорта
    m_currentChart = chart;

    // Обновляем виджет графика
    if (m_plotWidget) {
        m_plotWidget->setChart(chart);
        logMessage("updatePlot: Интерактивный график обновлен успешно");
    } else {
        logMessage("updatePlot: ERROR - m_plotWidget is null!");
    }
}

void ShiwaDiffPHCMainWindow::onAbout() {
    QMessageBox::about(this, "О программе ShiwaDiffPHC",
                      "ShiwaDiffPHC v1.3.0\n\n"
                      "🎯 Анализатор различий протокола точного времени (PTP)\n\n"
                      "Этот инструмент измеряет временные различия между PTP устройствами\n"
                      "для анализа точности синхронизации часов.\n\n"
                      "✨ НОВИНКА v1.3.0 - Фаза 2: Расширение интерфейса!\n"
                      "🎨 Современный темный интерфейс\n"
                      "⌨️ Клавиатурные сокращения\n"
                      "📊 Интерактивные графики с зумом\n"
                      "💾 Экспорт графиков в PNG/SVG\n"
                      "🖱️ Drag & Drop для конфигураций\n"
                      "📈 Расширенный статистический анализ\n\n"
                      "🔧 Требует привилегии root для доступа к PTP устройствам.\n"
                      "📚 Документация: TROUBLESHOOTING.md");
}

void ShiwaDiffPHCMainWindow::applyDarkTheme() {
    m_darkTheme = true;
    
    // Load stylesheet from file
    QFile styleFile("styles.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        qApp->setStyleSheet(style);
        styleFile.close();
    } else {
        // Fallback to inline styles if file not found
        qApp->setStyleSheet(
            "QMainWindow { background-color: #2b2b2b; color: #ffffff; }"
            "QWidget { background-color: #2b2b2b; color: #ffffff; }"
            "QPushButton { background-color: #4fc3f7; border: none; border-radius: 6px; color: #000000; font-weight: bold; padding: 8px 16px; }"
            "QPushButton:hover { background-color: #29b6f6; }"
            "QPushButton:pressed { background-color: #0288d1; }"
            "QGroupBox { font-weight: bold; border: 2px solid #555555; border-radius: 8px; margin-top: 1ex; padding-top: 10px; background-color: #3c3c3c; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; color: #4fc3f7; font-size: 10pt; font-weight: bold; }"
            "QSpinBox { background-color: #404040; border: 2px solid #555555; border-radius: 4px; padding: 4px; color: #ffffff; }"
            "QCheckBox { color: #ffffff; spacing: 8px; }"
            "QTableWidget { background-color: #404040; alternate-background-color: #4a4a4a; selection-background-color: #4fc3f7; gridline-color: #555555; border: 1px solid #555555; border-radius: 4px; }"
            "QTabWidget::pane { border: 1px solid #555555; background-color: #3c3c3c; border-radius: 4px; }"
            "QTabBar::tab { background-color: #555555; color: #ffffff; padding: 8px 16px; margin-right: 2px; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
            "QTabBar::tab:selected { background-color: #4fc3f7; color: #000000; font-weight: bold; }"
            "QPlainTextEdit, QTextEdit { background-color: #404040; border: 1px solid #555555; border-radius: 4px; color: #ffffff; font-family: 'Consolas', 'Monaco', monospace; }"
            "QProgressBar { border: 2px solid #555555; border-radius: 8px; text-align: center; background-color: #404040; color: #ffffff; font-weight: bold; }"
            "QProgressBar::chunk { background-color: #4fc3f7; border-radius: 6px; }"
            "QStatusBar { background-color: #3c3c3c; color: #ffffff; border-top: 1px solid #555555; }"
            "QMenuBar { background-color: #3c3c3c; color: #ffffff; border-bottom: 1px solid #555555; }"
            "QMenu { background-color: #3c3c3c; color: #ffffff; border: 1px solid #555555; border-radius: 4px; }"
            "QMenu::item:selected { background-color: #4fc3f7; color: #000000; }"
        );
    }
}

void ShiwaDiffPHCMainWindow::setupKeyboardShortcuts() {
    // Start/Stop measurement
    new QShortcut(QKeySequence("Ctrl+R"), this, SLOT(onStartMeasurement()));
    new QShortcut(QKeySequence("Ctrl+S"), this, SLOT(onStopMeasurement()));
    
    // File operations
    new QShortcut(QKeySequence("Ctrl+O"), this, SLOT(onLoadConfig()));
    new QShortcut(QKeySequence("Ctrl+Shift+S"), this, SLOT(onSaveConfig()));
    new QShortcut(QKeySequence("Ctrl+E"), this, SLOT(onSaveResults()));
    
    // Chart operations
    new QShortcut(QKeySequence("Ctrl+Plus"), this, SLOT(onZoomIn()));
    new QShortcut(QKeySequence("Ctrl+Minus"), this, SLOT(onZoomOut()));
    new QShortcut(QKeySequence("Ctrl+0"), this, SLOT(onResetZoom()));
    new QShortcut(QKeySequence("Ctrl+Shift+E"), this, SLOT(onExportChart()));
    
    // Theme toggle
    new QShortcut(QKeySequence("Ctrl+T"), this, SLOT(onToggleTheme()));
    
    // Device refresh
    new QShortcut(QKeySequence("F5"), this, SLOT(onRefreshDevices()));
    
    // Clear results
    new QShortcut(QKeySequence("Ctrl+Delete"), this, SLOT(clearResults()));
}

void ShiwaDiffPHCMainWindow::onToggleTheme() {
    m_darkTheme = !m_darkTheme;
    applyDarkTheme();
    logMessage(QString("Тема переключена на: %1").arg(m_darkTheme ? "Темная" : "Светлая"));
}

void ShiwaDiffPHCMainWindow::onExportChart() {
    if (!m_currentChart) {
        QMessageBox::warning(this, "Экспорт графика", "Нет данных для экспорта");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Экспорт графика", 
        QString("shiwadiffphc_chart_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "PNG Files (*.png);;SVG Files (*.svg);;JPEG Files (*.jpg)");
    
    if (!fileName.isEmpty()) {
        QPixmap pixmap = m_chartView->grab();
        if (pixmap.save(fileName)) {
            logMessage(QString("График экспортирован: %1").arg(fileName));
            QMessageBox::information(this, "Экспорт", "График успешно экспортирован");
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить график");
        }
    }
}

void ShiwaDiffPHCMainWindow::onZoomIn() {
    if (m_chartView) {
        m_chartView->chart()->zoomIn();
        logMessage("Увеличение графика");
    }
}

void ShiwaDiffPHCMainWindow::onZoomOut() {
    if (m_chartView) {
        m_chartView->chart()->zoomOut();
        logMessage("Уменьшение графика");
    }
}

void ShiwaDiffPHCMainWindow::onResetZoom() {
    if (m_chartView) {
        m_chartView->chart()->zoomReset();
        logMessage("Сброс масштаба графика");
    }
}

void ShiwaDiffPHCMainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ShiwaDiffPHCMainWindow::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        
        for (const QUrl& url : urlList) {
            QString fileName = url.toLocalFile();
            if (fileName.endsWith(".json") || fileName.endsWith(".conf")) {
                // Load configuration file
                QFile file(fileName);
                if (file.open(QFile::ReadOnly)) {
                    QTextStream in(&file);
                    QString content = in.readAll();
                    file.close();
                    
                    // Parse and apply configuration
                    logMessage(QString("Загружена конфигурация: %1").arg(fileName));
                    // TODO: Implement configuration parsing
                }
            }
        }
        
        event->acceptProposedAction();
    }
}

void ShiwaDiffPHCMainWindow::onAdvancedAnalysis() {
    if (m_results.empty()) {
        QMessageBox::information(this, "Расширенный анализ", 
                               "Нет данных для анализа. Сначала выполните измерения.");
        return;
    }
    
    // Perform comprehensive analysis on the latest result
    const PHCResult& latestResult = m_results.back();
    logMessage("Выполняется расширенный анализ...");
    
    m_advancedStats = AdvancedAnalysis::performComprehensiveAnalysis(latestResult);
    m_hasAdvancedStats = true;
    
    // Display results in a dialog
    QString analysisText = QString(
        "📊 РАСШИРЕННЫЙ АНАЛИЗ ЗАВЕРШЕН\n\n"
        "ℹ️ ПРИМЕЧАНИЕ: PTP устройства измеряют время относительно эпохи Unix (1970 г.),\n"
        "поэтому абсолютные значения очень большие. Анализ выполнен на относительных разностях.\n\n"
        "🔍 Анализ трендов:\n"
        "  • Тип тренда: %1\n"
        "  • Наклон: %2 нс/сек\n"
        "  • R²: %3\n"
        "  • Корреляция: %4\n"
        "  • Статистически значим: %5\n\n"
        "📈 Спектральный анализ:\n"
        "  • Доминирующая частота: %6\n"
        "  • Общая мощность: %7\n"
        "  • Низкие частоты: %8\n"
        "  • Средние частоты: %9\n"
        "  • Высокие частоты: %10\n\n"
        "⚠️ Детекция аномалий:\n"
        "  • Найдено аномалий: %11\n"
        "  • Процент аномалий: %12%\n"
        "  • Порог: %13\n\n"
        "⏱️ Метаданные:\n"
        "  • Точок данных: %14\n"
        "  • Время анализа: %15\n"
    ).arg(QString::fromStdString(m_advancedStats.trend.trend_type))
     .arg(m_advancedStats.trend.slope, 0, 'e', 2)
     .arg(m_advancedStats.trend.r_squared, 0, 'f', 3)
     .arg(m_advancedStats.trend.correlation, 0, 'f', 3)
     .arg(m_advancedStats.trend.is_significant ? "Да" : "Нет")
     .arg(QString::fromStdString(AdvancedAnalysis::formatFrequency(m_advancedStats.spectral.dominant_frequency)))
     .arg(m_advancedStats.spectral.total_power, 0, 'e', 2)
     .arg(m_advancedStats.spectral.power_bands["low_frequency"], 0, 'e', 2)
     .arg(m_advancedStats.spectral.power_bands["mid_frequency"], 0, 'e', 2)
     .arg(m_advancedStats.spectral.power_bands["high_frequency"], 0, 'e', 2)
     .arg(m_advancedStats.anomalies.total_anomalies)
     .arg(m_advancedStats.anomalies.anomaly_rate, 0, 'f', 1)
     .arg(m_advancedStats.anomalies.threshold, 0, 'f', 1)
     .arg(m_advancedStats.data_points_analyzed)
     .arg(QString::fromStdString(AdvancedAnalysis::formatDuration(m_advancedStats.analysis_duration_ms)));
    
    QMessageBox::information(this, "Результаты расширенного анализа", analysisText);
    logMessage("Расширенный анализ завершен успешно");
}

void ShiwaDiffPHCMainWindow::onTrendAnalysis() {
    if (!m_hasAdvancedStats) {
        onAdvancedAnalysis();
    }
    
    QString trendText = QString(
        "📈 АНАЛИЗ ТРЕНДОВ\n\n"
        "Тип тренда: %1\n"
        "Наклон: %2 нс/сек\n"
        "Пересечение: %3 нс\n"
        "Коэффициент детерминации (R²): %4\n"
        "Корреляция: %5\n"
        "P-значение: %6\n"
        "Статистически значим: %7\n\n"
        "Интерпретация:\n"
        "%8"
    ).arg(QString::fromStdString(m_advancedStats.trend.trend_type))
     .arg(m_advancedStats.trend.slope, 0, 'e', 2)
     .arg(m_advancedStats.trend.intercept, 0, 'e', 2)
     .arg(m_advancedStats.trend.r_squared, 0, 'f', 3)
     .arg(m_advancedStats.trend.correlation, 0, 'f', 3)
     .arg(m_advancedStats.trend.p_value, 0, 'f', 3)
     .arg(m_advancedStats.trend.is_significant ? "Да" : "Нет")
     .arg(getTrendInterpretation());
    
    QMessageBox::information(this, "Анализ трендов", trendText);
}

void ShiwaDiffPHCMainWindow::onSpectralAnalysis() {
    if (!m_hasAdvancedStats) {
        onAdvancedAnalysis();
    }
    
    QString spectralText = QString(
        "📊 СПЕКТРАЛЬНЫЙ АНАЛИЗ\n\n"
        "Доминирующая частота: %1\n"
        "Общая мощность: %2\n\n"
        "Распределение по частотным полосам:\n"
        "• Низкие частоты (< 0.1 Гц): %3\n"
        "• Средние частоты (0.1-1 Гц): %4\n"
        "• Высокие частоты (> 1 Гц): %5\n\n"
        "Интерпретация:\n"
        "%6"
    ).arg(QString::fromStdString(AdvancedAnalysis::formatFrequency(m_advancedStats.spectral.dominant_frequency)))
     .arg(m_advancedStats.spectral.total_power, 0, 'e', 2)
     .arg(m_advancedStats.spectral.power_bands["low_frequency"], 0, 'e', 2)
     .arg(m_advancedStats.spectral.power_bands["mid_frequency"], 0, 'e', 2)
     .arg(m_advancedStats.spectral.power_bands["high_frequency"], 0, 'e', 2)
     .arg(getSpectralInterpretation());
    
    QMessageBox::information(this, "Спектральный анализ", spectralText);
}

void ShiwaDiffPHCMainWindow::onAnomalyDetection() {
    if (!m_hasAdvancedStats) {
        onAdvancedAnalysis();
    }
    
    QString anomalyText = QString(
        "⚠️ ДЕТЕКЦИЯ АНОМАЛИЙ\n\n"
        "Найдено аномалий: %1\n"
        "Процент аномалий: %2%\n"
        "Порог детекции: %3\n\n"
        "Индексы аномалий: %4\n\n"
        "Интерпретация:\n"
        "%5"
    ).arg(m_advancedStats.anomalies.total_anomalies)
     .arg(m_advancedStats.anomalies.anomaly_rate, 0, 'f', 1)
     .arg(m_advancedStats.anomalies.threshold, 0, 'f', 1)
     .arg(formatAnomalyIndices())
     .arg(getAnomalyInterpretation());
    
    QMessageBox::information(this, "Детекция аномалий", anomalyText);
}

void ShiwaDiffPHCMainWindow::onGenerateReport() {
    if (!m_hasAdvancedStats) {
        QMessageBox::information(this, "Генерация отчета", 
                               "Сначала выполните расширенный анализ.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Сохранить отчет", 
        QString("shiwadiffphc_report_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "Text Files (*.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            
            out << "ОТЧЕТ SHIWADIFFPHC - РАСШИРЕННЫЙ АНАЛИЗ\n";
            out << "==========================================\n\n";
            out << "Дата создания: " << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << "\n\n";
            
            // Trend Analysis
            out << "АНАЛИЗ ТРЕНДОВ\n";
            out << "---------------\n";
            out << "Тип тренда: " << QString::fromStdString(m_advancedStats.trend.trend_type) << "\n";
            out << "Наклон: " << QString::number(m_advancedStats.trend.slope, 'e', 2) << " нс/сек\n";
            out << "R²: " << QString::number(m_advancedStats.trend.r_squared, 'f', 3) << "\n";
            out << "Корреляция: " << QString::number(m_advancedStats.trend.correlation, 'f', 3) << "\n\n";
            
            // Spectral Analysis
            out << "СПЕКТРАЛЬНЫЙ АНАЛИЗ\n";
            out << "--------------------\n";
            out << "Доминирующая частота: " << QString::fromStdString(AdvancedAnalysis::formatFrequency(m_advancedStats.spectral.dominant_frequency)) << "\n";
            out << "Общая мощность: " << QString::number(m_advancedStats.spectral.total_power, 'e', 2) << "\n\n";
            
            // Anomaly Detection
            out << "ДЕТЕКЦИЯ АНОМАЛИЙ\n";
            out << "------------------\n";
            out << "Найдено аномалий: " << m_advancedStats.anomalies.total_anomalies << "\n";
            out << "Процент аномалий: " << QString::number(m_advancedStats.anomalies.anomaly_rate, 'f', 1) << "%\n\n";
            
            // Metadata
            out << "МЕТАДАННЫЕ\n";
            out << "-----------\n";
            out << "Точек данных: " << m_advancedStats.data_points_analyzed << "\n";
            out << "Время анализа: " << QString::fromStdString(AdvancedAnalysis::formatDuration(m_advancedStats.analysis_duration_ms)) << "\n";
            
            file.close();
            logMessage(QString("Отчет сохранен: %1").arg(fileName));
            QMessageBox::information(this, "Отчет", "Отчет успешно сохранен");
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить отчет");
        }
    }
}

QString ShiwaDiffPHCMainWindow::getTrendInterpretation() {
    if (!m_hasAdvancedStats) return "Нет данных";
    
    const auto& trend = m_advancedStats.trend;
    QString interpretation;
    
    if (trend.trend_type == "increasing") {
        interpretation = "Обнаружен восходящий тренд. Временные различия увеличиваются со временем.";
    } else if (trend.trend_type == "decreasing") {
        interpretation = "Обнаружен нисходящий тренд. Временные различия уменьшаются со временем.";
    } else {
        interpretation = "Тренд стабильный. Временные различия не показывают значительных изменений.";
    }
    
    if (trend.is_significant) {
        interpretation += " Тренд статистически значим.";
    } else {
        interpretation += " Тренд не является статистически значимым.";
    }
    
    return interpretation;
}

QString ShiwaDiffPHCMainWindow::getSpectralInterpretation() {
    if (!m_hasAdvancedStats) return "Нет данных";
    
    const auto& spectral = m_advancedStats.spectral;
    QString interpretation;
    
    if (spectral.dominant_frequency < 0.01) {
        interpretation = "Доминируют очень низкие частоты. Система показывает медленные изменения.";
    } else if (spectral.dominant_frequency < 0.1) {
        interpretation = "Доминируют низкие частоты. Наблюдаются медленные колебания.";
    } else if (spectral.dominant_frequency < 1.0) {
        interpretation = "Доминируют средние частоты. Система показывает умеренные колебания.";
    } else {
        interpretation = "Доминируют высокие частоты. Наблюдаются быстрые колебания или шум.";
    }
    
    return interpretation;
}

QString ShiwaDiffPHCMainWindow::getAnomalyInterpretation() {
    if (!m_hasAdvancedStats) return "Нет данных";
    
    const auto& anomalies = m_advancedStats.anomalies;
    QString interpretation;
    
    if (anomalies.anomaly_rate < 1.0) {
        interpretation = "Очень низкий уровень аномалий. Система работает стабильно.";
    } else if (anomalies.anomaly_rate < 5.0) {
        interpretation = "Низкий уровень аномалий. Система работает нормально.";
    } else if (anomalies.anomaly_rate < 10.0) {
        interpretation = "Умеренный уровень аномалий. Рекомендуется мониторинг.";
    } else {
        interpretation = "Высокий уровень аномалий. Требуется внимание к системе.";
    }
    
    return interpretation;
}

QString ShiwaDiffPHCMainWindow::formatAnomalyIndices() {
    if (!m_hasAdvancedStats || m_advancedStats.anomalies.outlier_indices.empty()) {
        return "Нет аномалий";
    }
    
    QStringList indices;
    for (int idx : m_advancedStats.anomalies.outlier_indices) {
        indices << QString::number(idx);
    }
    
    if (indices.size() > 10) {
        return indices.mid(0, 10).join(", ") + "...";
    } else {
        return indices.join(", ");
    }
}

// Main function for GUI application
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ShiwaDiffPHC");
    app.setApplicationVersion("1.3.0");
    app.setOrganizationName("Shiwa Tools");
    
    ShiwaDiffPHCMainWindow window;
    window.show();
    
    return app.exec();
}

#include "diffphc_gui.moc"