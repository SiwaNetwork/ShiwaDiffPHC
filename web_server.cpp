#include "web_server.h"
#include <QNetworkInterface>
#include <QHostAddress>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

WebServer::WebServer(QObject *parent)
    : QObject(parent)
    , m_httpServer(nullptr)
    , m_broadcastTimer(new QTimer(this))
    , m_running(false)
    , m_port(8080)
    , m_measuring(false)
{
    m_httpServer = new QHttpServer(this);
    setupRoutes();
    
    // Setup broadcast timer for real-time updates
    connect(m_broadcastTimer, &QTimer::timeout, this, &WebServer::broadcastUpdate);
    m_broadcastTimer->setInterval(1000); // Update every second
}

WebServer::~WebServer()
{
    stopServer();
}

bool WebServer::startServer(quint16 port)
{
    if (m_running) {
        return true;
    }
    
    m_port = port;
    
    // Try to start the server
    auto result = m_httpServer->listen(QHostAddress::Any, port);
    if (result == -1) {
        qWarning() << "Failed to start web server on port" << port;
        return false;
    }
    
    m_running = true;
    m_broadcastTimer->start();
    
    qDebug() << "Web server started on port" << port;
    qDebug() << "Access the web interface at: http://localhost:" << port;
    
    return true;
}

void WebServer::stopServer()
{
    if (!m_running) {
        return;
    }
    
    m_broadcastTimer->stop();
    m_httpServer->deleteLater();
    m_httpServer = nullptr;
    m_running = false;
    
    qDebug() << "Web server stopped";
}

bool WebServer::isRunning() const
{
    return m_running;
}

quint16 WebServer::getPort() const
{
    return m_port;
}

void WebServer::addMeasurementResult(const PHCResult& result)
{
    QMutexLocker locker(&m_dataMutex);
    m_measurementHistory.push_back(result);
    
    // Keep only last 1000 measurements to prevent memory issues
    if (m_measurementHistory.size() > 1000) {
        m_measurementHistory.erase(m_measurementHistory.begin(), m_measurementHistory.begin() + 100);
    }
    
    m_lastUpdate = QDateTime::currentDateTime();
}

void WebServer::setCurrentConfig(const PHCConfig& config)
{
    QMutexLocker locker(&m_dataMutex);
    m_currentConfig = config;
}

void WebServer::setAvailableDevices(const std::vector<int>& devices)
{
    QMutexLocker locker(&m_dataMutex);
    m_availableDevices = devices;
}

void WebServer::setMeasurementStatus(bool measuring)
{
    QMutexLocker locker(&m_dataMutex);
    m_measuring = measuring;
}

void WebServer::setupRoutes()
{
    // Main web interface
    m_httpServer->route("/", [this](const QHttpServerRequest &request) {
        return QHttpServerResponse("text/html", generateWebInterface().toUtf8());
    });
    
    // API endpoints
    m_httpServer->route("/api/status", [this](const QHttpServerRequest &request) {
        return QHttpServerResponse("application/json", createJsonResponse(getCurrentStatus()).toUtf8());
    });
    
    m_httpServer->route("/api/history", [this](const QHttpServerRequest &request) {
        return QHttpServerResponse("application/json", createJsonResponse(QJsonObject{{"history", getMeasurementHistory()}}).toUtf8());
    });
    
    m_httpServer->route("/api/devices", [this](const QHttpServerRequest &request) {
        QMutexLocker locker(&m_dataMutex);
        QJsonArray devices;
        for (int device : m_availableDevices) {
            devices.append(device);
        }
        return QHttpServerResponse("application/json", createJsonResponse(QJsonObject{{"devices", devices}}).toUtf8());
    });
    
    // Control endpoints
    m_httpServer->route("/api/start", [this](const QHttpServerRequest &request) {
        emit measurementRequested(m_currentConfig);
        return QHttpServerResponse("application/json", createJsonResponse(QJsonObject{{"status", "started"}}).toUtf8());
    });
    
    m_httpServer->route("/api/stop", [this](const QHttpServerRequest &request) {
        emit measurementStopped();
        return QHttpServerResponse("application/json", createJsonResponse(QJsonObject{{"status", "stopped"}}).toUtf8());
    });
    
    m_httpServer->route("/api/refresh", [this](const QHttpServerRequest &request) {
        emit deviceRefreshRequested();
        return QHttpServerResponse("application/json", createJsonResponse(QJsonObject{{"status", "refreshed"}}).toUtf8());
    });
}

QByteArray WebServer::createResponse(const QString& content, const QString& contentType)
{
    QByteArray response = content.toUtf8();
    return response;
}

QByteArray WebServer::createJsonResponse(const QJsonObject& json)
{
    QJsonDocument doc(json);
    return doc.toJson();
}

QByteArray WebServer::createCorsHeaders()
{
    return QByteArray("Access-Control-Allow-Origin: *\r\n"
                     "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                     "Access-Control-Allow-Headers: Content-Type\r\n");
}

QString WebServer::generateWebInterface()
{
    return R"(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ShiwaDiffPHC - Веб-интерфейс</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            min-height: 100vh;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        
        .header p {
            font-size: 1.2em;
            opacity: 0.9;
        }
        
        .dashboard {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .card {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 15px;
            padding: 25px;
            border: 1px solid rgba(255, 255, 255, 0.2);
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
        }
        
        .card h3 {
            margin-bottom: 20px;
            font-size: 1.4em;
            color: #4fc3f7;
        }
        
        .controls {
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
            margin-bottom: 20px;
        }
        
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 8px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .btn-primary {
            background: linear-gradient(45deg, #4fc3f7, #29b6f6);
            color: white;
        }
        
        .btn-danger {
            background: linear-gradient(45deg, #f44336, #d32f2f);
            color: white;
        }
        
        .btn-success {
            background: linear-gradient(45deg, #4caf50, #388e3c);
            color: white;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2);
        }
        
        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
            transform: none;
        }
        
        .status {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 20px;
        }
        
        .status-indicator {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background: #4caf50;
            animation: pulse 2s infinite;
        }
        
        .status-indicator.stopped {
            background: #f44336;
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
        
        .chart-container {
            position: relative;
            height: 400px;
            margin-top: 20px;
        }
        
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-top: 20px;
        }
        
        .stat-item {
            background: rgba(255, 255, 255, 0.05);
            padding: 15px;
            border-radius: 10px;
            text-align: center;
        }
        
        .stat-value {
            font-size: 1.8em;
            font-weight: bold;
            color: #4fc3f7;
        }
        
        .stat-label {
            font-size: 0.9em;
            opacity: 0.8;
            margin-top: 5px;
        }
        
        .log-container {
            max-height: 300px;
            overflow-y: auto;
            background: rgba(0, 0, 0, 0.3);
            border-radius: 10px;
            padding: 15px;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
        }
        
        .log-entry {
            margin-bottom: 5px;
            padding: 5px;
            border-radius: 5px;
        }
        
        .log-info { background: rgba(33, 150, 243, 0.2); }
        .log-success { background: rgba(76, 175, 80, 0.2); }
        .log-warning { background: rgba(255, 193, 7, 0.2); }
        .log-error { background: rgba(244, 67, 54, 0.2); }
        
        @media (max-width: 768px) {
            .dashboard {
                grid-template-columns: 1fr;
            }
            
            .controls {
                flex-direction: column;
            }
            
            .header h1 {
                font-size: 2em;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚀 ShiwaDiffPHC</h1>
            <p>Веб-интерфейс для мониторинга PTP устройств</p>
        </div>
        
        <div class="dashboard">
            <div class="card">
                <h3>🎛️ Управление</h3>
                <div class="status">
                    <div class="status-indicator" id="statusIndicator"></div>
                    <span id="statusText">Остановлено</span>
                </div>
                <div class="controls">
                    <button class="btn btn-primary" id="startBtn" onclick="startMeasurement()">▶️ Запустить</button>
                    <button class="btn btn-danger" id="stopBtn" onclick="stopMeasurement()" disabled>⏹️ Остановить</button>
                    <button class="btn btn-success" onclick="refreshDevices()">🔄 Обновить</button>
                </div>
                <div class="stats-grid">
                    <div class="stat-item">
                        <div class="stat-value" id="deviceCount">0</div>
                        <div class="stat-label">Устройств</div>
                    </div>
                    <div class="stat-item">
                        <div class="stat-value" id="measurementCount">0</div>
                        <div class="stat-label">Измерений</div>
                    </div>
                    <div class="stat-item">
                        <div class="stat-value" id="avgDifference">0</div>
                        <div class="stat-label">Ср. разность (нс)</div>
                    </div>
                </div>
            </div>
            
            <div class="card">
                <h3>📊 График в реальном времени</h3>
                <div class="chart-container">
                    <canvas id="timeChart"></canvas>
                </div>
            </div>
        </div>
        
        <div class="card">
            <h3>📝 Лог событий</h3>
            <div class="log-container" id="logContainer">
                <div class="log-entry log-info">Веб-интерфейс инициализирован</div>
            </div>
        </div>
    </div>

    <script>
        let chart;
        let isMeasuring = false;
        
        // Initialize Chart.js
        function initChart() {
            const ctx = document.getElementById('timeChart').getContext('2d');
            chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: 'Разность времени (нс)',
                        data: [],
                        borderColor: '#4fc3f7',
                        backgroundColor: 'rgba(79, 195, 247, 0.1)',
                        borderWidth: 2,
                        fill: true,
                        tension: 0.4
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        y: {
                            beginAtZero: false,
                            grid: {
                                color: 'rgba(255, 255, 255, 0.1)'
                            },
                            ticks: {
                                color: '#fff'
                            }
                        },
                        x: {
                            grid: {
                                color: 'rgba(255, 255, 255, 0.1)'
                            },
                            ticks: {
                                color: '#fff'
                            }
                        }
                    },
                    plugins: {
                        legend: {
                            labels: {
                                color: '#fff'
                            }
                        }
                    }
                }
            });
        }
        
        // API functions
        async function fetchData(url) {
            try {
                const response = await fetch(url);
                return await response.json();
            } catch (error) {
                console.error('API Error:', error);
                addLog('Ошибка API: ' + error.message, 'error');
            }
        }
        
        async function updateStatus() {
            const status = await fetchData('/api/status');
            if (status) {
                updateUI(status);
            }
        }
        
        async function updateChart() {
            const data = await fetchData('/api/history');
            if (data && data.history) {
                updateChartData(data.history);
            }
        }
        
        function updateUI(status) {
            const statusIndicator = document.getElementById('statusIndicator');
            const statusText = document.getElementById('statusText');
            const startBtn = document.getElementById('startBtn');
            const stopBtn = document.getElementById('stopBtn');
            
            isMeasuring = status.measuring;
            
            if (isMeasuring) {
                statusIndicator.classList.remove('stopped');
                statusText.textContent = 'Измерение...';
                startBtn.disabled = true;
                stopBtn.disabled = false;
            } else {
                statusIndicator.classList.add('stopped');
                statusText.textContent = 'Остановлено';
                startBtn.disabled = false;
                stopBtn.disabled = true;
            }
            
            document.getElementById('deviceCount').textContent = status.deviceCount || 0;
            document.getElementById('measurementCount').textContent = status.measurementCount || 0;
            document.getElementById('avgDifference').textContent = (status.avgDifference || 0).toFixed(2);
        }
        
        function updateChartData(history) {
            if (!chart || !history.length) return;
            
            const labels = [];
            const data = [];
            
            history.slice(-50).forEach((item, index) => {
                labels.push(new Date(item.timestamp).toLocaleTimeString());
                data.push(item.difference || 0);
            });
            
            chart.data.labels = labels;
            chart.data.datasets[0].data = data;
            chart.update('none');
        }
        
        function addLog(message, type = 'info') {
            const logContainer = document.getElementById('logContainer');
            const logEntry = document.createElement('div');
            logEntry.className = `log-entry log-${type}`;
            logEntry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
            
            logContainer.appendChild(logEntry);
            logContainer.scrollTop = logContainer.scrollHeight;
            
            // Keep only last 100 log entries
            while (logContainer.children.length > 100) {
                logContainer.removeChild(logContainer.firstChild);
            }
        }
        
        // Control functions
        async function startMeasurement() {
            const result = await fetchData('/api/start');
            if (result) {
                addLog('Измерение запущено', 'success');
            }
        }
        
        async function stopMeasurement() {
            const result = await fetchData('/api/stop');
            if (result) {
                addLog('Измерение остановлено', 'warning');
            }
        }
        
        async function refreshDevices() {
            const result = await fetchData('/api/refresh');
            if (result) {
                addLog('Список устройств обновлен', 'info');
                updateStatus();
            }
        }
        
        // Initialize
        document.addEventListener('DOMContentLoaded', function() {
            initChart();
            updateStatus();
            updateChart();
            
            // Update data every second
            setInterval(() => {
                updateStatus();
                updateChart();
            }, 1000);
            
            addLog('Веб-интерфейс готов к работе', 'success');
        });
    </script>
</body>
</html>
    )";
}

QJsonObject WebServer::getCurrentStatus()
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonObject status;
    status["measuring"] = m_measuring;
    status["deviceCount"] = static_cast<int>(m_availableDevices.size());
    status["measurementCount"] = static_cast<int>(m_measurementHistory.size());
    status["lastUpdate"] = m_lastUpdate.toString(Qt::ISODate);
    
    // Calculate average difference
    double avgDiff = 0.0;
    if (!m_measurementHistory.empty()) {
        double sum = 0.0;
        int count = 0;
        for (const auto& result : m_measurementHistory) {
            if (result.success) {
                sum += result.difference;
                count++;
            }
        }
        if (count > 0) {
            avgDiff = sum / count;
        }
    }
    status["avgDifference"] = avgDiff;
    
    return status;
}

QJsonArray WebServer::getMeasurementHistory()
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonArray history;
    for (const auto& result : m_measurementHistory) {
        QJsonObject item;
        item["timestamp"] = result.timestamp.toString(Qt::ISODate);
        item["difference"] = result.difference;
        item["success"] = result.success;
        item["device"] = result.device;
        history.append(item);
    }
    
    return history;
}

void WebServer::broadcastUpdate()
{
    // This would be used for WebSocket real-time updates
    // For now, we rely on client-side polling
}
