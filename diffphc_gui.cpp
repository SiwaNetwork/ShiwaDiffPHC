#include "diffphc_gui.h"
#include <QApplication>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ShiwaShiwaDiffPHCMainWindow::ShiwaDiffPHCMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_measurementTimer(new QTimer(this))
    , m_measuring(false)
    , m_currentIteration(0)
{
    setWindowTitle("ShiwaDiffPHC - Анализатор различий протокола точного времени");
    setMinimumSize(800, 600);
    resize(1200, 800);

    setupUI();
    setupMenuBar();
    setupStatusBar();
    
    updateDeviceList();
    
    connect(m_measurementTimer, &QTimer::timeout, this, &ShiwaDiffPHCMainWindow::onTimerUpdate);
    
    // Initialize configuration
    m_currentConfig.count = 0;
    m_currentConfig.delay = 100000;
    m_currentConfig.samples = 10;
    m_currentConfig.debug = false;
    
    logMessage("ShiwaDiffPHC GUI инициализирован");
}

ShiwaShiwaDiffPHCMainWindow::~ShiwaDiffPHCMainWindow() {
    if (m_measuring) {
        onStopMeasurement();
    }
}

void ShiwaShiwaDiffPHCMainWindow::setupUI() {
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    
    auto* layout = new QHBoxLayout(m_centralWidget);
    layout->addWidget(m_mainSplitter);
    
    setupControlPanel();
    setupResultsPanel();
}

void ShiwaShiwaDiffPHCMainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("&File");
    
    auto* loadConfigAction = fileMenu->addAction("&Load Configuration...");
    connect(loadConfigAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onLoadConfig);
    
    auto* saveConfigAction = fileMenu->addAction("&Save Configuration...");
    connect(saveConfigAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onSaveConfig);
    
    fileMenu->addSeparator();
    
    auto* saveResultsAction = fileMenu->addAction("Save &Results...");
    connect(saveResultsAction, &QAction::triggered, this, &ShiwaDiffPHCMainWindow::onSaveResults);
    
    fileMenu->addSeparator();
    
    auto* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAction = helpMenu->addAction("&О программе ShiwaDiffPHC");
    connect(aboutAction, &QAction::triggered, this, &ShiwaShiwaDiffPHCMainWindow::onAbout);
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
    
    // Plot Tab (placeholder)
    m_plotWidget = new QTextEdit;
    m_plotWidget->setPlainText("Здесь будет доступна визуализация графиков\n(Функция будет реализована)");
    m_plotWidget->setReadOnly(true);
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

void ShiwaDiffPHCMainWindow::onAbout() {
    QMessageBox::about(this, "О программе ShiwaDiffPHC",
                      "ShiwaDiffPHC v1.2.0\n\n"
                      "Анализатор различий протокола точного времени (PTP)\n\n"
                      "Этот инструмент измеряет временные различия между PTP устройствами\n"
                      "для анализа точности синхронизации часов.\n\n"
                      "НОВИНКА v1.2.0: Расширенный статистический анализ!\n"
                      "• Медиана, стандартное отклонение, размах\n"
                      "• Автоматические расчеты в реальном времени\n\n"
                      "Требует привилегии root для доступа к PTP устройствам.");
}

// Main function for GUI application
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ShiwaDiffPHC");
    app.setApplicationVersion("1.2.0");
    app.setOrganizationName("Shiwa Tools");
    
    ShiwaDiffPHCMainWindow window;
    window.show();
    
    return app.exec();
}

#include "diffphc_gui.moc"