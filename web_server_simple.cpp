#include "web_server_simple.h"
#include <QHostAddress>
#include <QDebug>

WebServerSimple::WebServerSimple(QObject *parent)
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
    connect(m_broadcastTimer, &QTimer::timeout, this, &WebServerSimple::broadcastUpdate);
    m_broadcastTimer->setInterval(1000); // Update every second
}

WebServerSimple::~WebServerSimple()
{
    stopServer();
}

bool WebServerSimple::startServer(quint16 port)
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

void WebServerSimple::stopServer()
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

bool WebServerSimple::isRunning() const
{
    return m_running;
}

quint16 WebServerSimple::getPort() const
{
    return m_port;
}

void WebServerSimple::addMeasurementResult(const PHCResult& result)
{
    QMutexLocker locker(&m_dataMutex);
    m_measurementHistory.push_back(result);
    
    // Keep only last 1000 measurements to prevent memory issues
    if (m_measurementHistory.size() > 1000) {
        m_measurementHistory.erase(m_measurementHistory.begin(), m_measurementHistory.begin() + 100);
    }
    
    m_lastUpdate = QDateTime::currentDateTime();
}

void WebServerSimple::setCurrentConfig(const PHCConfig& config)
{
    QMutexLocker locker(&m_dataMutex);
    m_currentConfig = config;
}

void WebServerSimple::setAvailableDevices(const std::vector<int>& devices)
{
    QMutexLocker locker(&m_dataMutex);
    m_availableDevices = devices;
}

void WebServerSimple::setMeasurementStatus(bool measuring)
{
    QMutexLocker locker(&m_dataMutex);
    m_measuring = measuring;
}

void WebServerSimple::setupRoutes()
{
    // Main web interface
    m_httpServer->route("/", [this](const QHttpServerRequest &request) {
        return QHttpServerResponse("text/html", generateSimpleWebInterface().toUtf8());
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

QByteArray WebServerSimple::createResponse(const QString& content, const QString& contentType)
{
    return content.toUtf8();
}

QByteArray WebServerSimple::createJsonResponse(const QJsonObject& json)
{
    QJsonDocument doc(json);
    return doc.toJson();
}

QString WebServerSimple::generateSimpleWebInterface()
{
    return QString(R"(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ShiwaDiffPHC - Веб-интерфейс</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: #1e3c72;
            color: white;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        .card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .btn {
            padding: 10px 20px;
            margin: 5px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-weight: bold;
        }
        .btn-primary { background: #4fc3f7; color: white; }
        .btn-danger { background: #f44336; color: white; }
        .btn-success { background: #4caf50; color: white; }
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
        }
        .status-indicator.stopped { background: #f44336; }
        .stats {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-top: 20px;
        }
        .stat-item {
            background: rgba(255, 255, 255, 0.05);
            padding: 15px;
            border-radius: 8px;
            text-align: center;
        }
        .stat-value {
            font-size: 1.5em;
            font-weight: bold;
            color: #4fc3f7;
        }
        .log {
            max-height: 300px;
            overflow-y: auto;
            background: rgba(0, 0, 0, 0.3);
            border-radius: 8px;
            padding: 15px;
            font-family: monospace;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ShiwaDiffPHC - Веб-интерфейс</h1>
            <p>Мониторинг PTP устройств</p>
        </div>
        
        <div class="card">
            <h3>Управление</h3>
            <div class="status">
                <div class="status-indicator" id="statusIndicator"></div>
                <span id="statusText">Остановлено</span>
            </div>
            <button class="btn btn-primary" id="startBtn" onclick="startMeasurement()">Запустить</button>
            <button class="btn btn-danger" id="stopBtn" onclick="stopMeasurement()" disabled>Остановить</button>
            <button class="btn btn-success" onclick="refreshDevices()">Обновить</button>
            
            <div class="stats">
                <div class="stat-item">
                    <div class="stat-value" id="deviceCount">0</div>
                    <div>Устройств</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value" id="measurementCount">0</div>
                    <div>Измерений</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value" id="avgDifference">0</div>
                    <div>Ср. разность (нс)</div>
                </div>
            </div>
        </div>
        
        <div class="card">
            <h3>Лог событий</h3>
            <div class="log" id="logContainer">
                <div>Веб-интерфейс инициализирован</div>
            </div>
        </div>
    </div>

    <script>
        let isMeasuring = false;
        
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
        
        function addLog(message, type = 'info') {
            const logContainer = document.getElementById('logContainer');
            const logEntry = document.createElement('div');
            logEntry.textContent = '[' + new Date().toLocaleTimeString() + '] ' + message;
            
            logContainer.appendChild(logEntry);
            logContainer.scrollTop = logContainer.scrollHeight;
            
            // Keep only last 50 log entries
            while (logContainer.children.length > 50) {
                logContainer.removeChild(logContainer.firstChild);
            }
        }
        
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
            updateStatus();
            
            // Update data every second
            setInterval(() => {
                updateStatus();
            }, 1000);
            
            addLog('Веб-интерфейс готов к работе', 'success');
        });
    </script>
</body>
</html>
    )");
}

QJsonObject WebServerSimple::getCurrentStatus()
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
            if (result.success && !result.differences.empty() && !result.differences[0].empty()) {
                sum += result.differences[0][0]; // Use first difference as example
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

QJsonArray WebServerSimple::getMeasurementHistory()
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonArray history;
    for (const auto& result : m_measurementHistory) {
        QJsonObject item;
        item["timestamp"] = QDateTime::fromMSecsSinceEpoch(result.baseTimestamp / 1000000).toString(Qt::ISODate);
        if (!result.differences.empty() && !result.differences[0].empty()) {
            item["difference"] = result.differences[0][0];
        } else {
            item["difference"] = 0;
        }
        item["success"] = result.success;
        item["deviceCount"] = static_cast<int>(result.devices.size());
        history.append(item);
    }
    
    return history;
}

void WebServerSimple::broadcastUpdate()
{
    // This would be used for WebSocket real-time updates
    // For now, we rely on client-side polling
}
