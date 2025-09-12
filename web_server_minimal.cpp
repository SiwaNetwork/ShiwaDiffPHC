#include "web_server_minimal.h"
#include <QHostAddress>
#include <QDebug>
#include <QTextStream>

WebServerMinimal::WebServerMinimal(QObject *parent)
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
    connect(m_broadcastTimer, &QTimer::timeout, this, &WebServerMinimal::broadcastUpdate);
    m_broadcastTimer->setInterval(1000); // Update every second
}

WebServerMinimal::~WebServerMinimal()
{
    stopServer();
}

bool WebServerMinimal::startServer(quint16 port)
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

void WebServerMinimal::stopServer()
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

bool WebServerMinimal::isRunning() const
{
    return m_running;
}

quint16 WebServerMinimal::getPort() const
{
    return m_port;
}

void WebServerMinimal::addMeasurementResult(const PHCResult& result)
{
    QMutexLocker locker(&m_dataMutex);
    m_measurementHistory.push_back(result);
    
    // Keep only last 1000 measurements to prevent memory issues
    if (m_measurementHistory.size() > 1000) {
        m_measurementHistory.erase(m_measurementHistory.begin(), m_measurementHistory.begin() + 100);
    }
    
    m_lastUpdate = QDateTime::currentDateTime();
}

void WebServerMinimal::setCurrentConfig(const PHCConfig& config)
{
    QMutexLocker locker(&m_dataMutex);
    m_currentConfig = config;
}

void WebServerMinimal::setAvailableDevices(const std::vector<int>& devices)
{
    QMutexLocker locker(&m_dataMutex);
    m_availableDevices = devices;
}

void WebServerMinimal::setMeasurementStatus(bool measuring)
{
    QMutexLocker locker(&m_dataMutex);
    m_measuring = measuring;
}

void WebServerMinimal::setupRoutes()
{
    // Main web interface
    m_httpServer->route("/", [this](const QHttpServerRequest &request) {
        return QHttpServerResponse("text/html", loadWebInterface().toUtf8());
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

QByteArray WebServerMinimal::createResponse(const QString& content, const QString& contentType)
{
    return content.toUtf8();
}

QByteArray WebServerMinimal::createJsonResponse(const QJsonObject& json)
{
    QJsonDocument doc(json);
    return doc.toJson();
}

QString WebServerMinimal::loadWebInterface()
{
    QFile file("web_interface.html");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open web_interface.html file";
        return QString("<html><body><h1>Error: Cannot load web interface</h1></body></html>");
    }
    
    QTextStream in(&file);
    return in.readAll();
}

QJsonObject WebServerMinimal::getCurrentStatus()
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
                sum += static_cast<double>(result.differences[0][0]); // Use first difference as example
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

QJsonArray WebServerMinimal::getMeasurementHistory()
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonArray history;
    for (const auto& result : m_measurementHistory) {
        QJsonObject item;
        item["timestamp"] = QDateTime::fromMSecsSinceEpoch(result.baseTimestamp / 1000000).toString(Qt::ISODate);
        if (!result.differences.empty() && !result.differences[0].empty()) {
            item["difference"] = static_cast<double>(result.differences[0][0]);
        } else {
            item["difference"] = 0.0;
        }
        item["success"] = result.success;
        item["deviceCount"] = static_cast<int>(result.devices.size());
        history.append(item);
    }
    
    return history;
}

void WebServerMinimal::broadcastUpdate()
{
    // This would be used for WebSocket real-time updates
    // For now, we rely on client-side polling
}
