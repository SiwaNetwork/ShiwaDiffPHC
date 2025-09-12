#include "web_server_alternative.h"
#include <QHostAddress>
#include <QDebug>
#include <QTextStream>
#include <QRegularExpression>

WebServerAlternative::WebServerAlternative(QObject *parent)
    : QObject(parent)
    , m_tcpServer(nullptr)
    , m_broadcastTimer(new QTimer(this))
    , m_running(false)
    , m_port(8080)
    , m_measuring(false)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &WebServerAlternative::onNewConnection);
    
    // Setup broadcast timer for real-time updates
    connect(m_broadcastTimer, &QTimer::timeout, this, &WebServerAlternative::broadcastUpdate);
    m_broadcastTimer->setInterval(1000); // Update every second
}

WebServerAlternative::~WebServerAlternative()
{
    stopServer();
}

bool WebServerAlternative::startServer(quint16 port)
{
    if (m_running) {
        return true;
    }
    
    m_port = port;
    
    // Try to start the server
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        qWarning() << "Failed to start web server on port" << port << ":" << m_tcpServer->errorString();
        return false;
    }
    
    m_running = true;
    m_broadcastTimer->start();
    
    qDebug() << "Web server started on port" << port;
    qDebug() << "Access the web interface at: http://localhost:" << port;
    
    return true;
}

void WebServerAlternative::stopServer()
{
    if (!m_running) {
        return;
    }
    
    m_broadcastTimer->stop();
    
    // Close all client connections
    for (QTcpSocket* client : m_clients) {
        client->disconnectFromHost();
    }
    m_clients.clear();
    
    m_tcpServer->close();
    m_running = false;
    
    qDebug() << "Web server stopped";
}

bool WebServerAlternative::isRunning() const
{
    return m_running;
}

quint16 WebServerAlternative::getPort() const
{
    return m_port;
}

void WebServerAlternative::addMeasurementResult(const PHCResult& result)
{
    QMutexLocker locker(&m_dataMutex);
    m_measurementHistory.push_back(result);
    
    // Keep only last 1000 measurements to prevent memory issues
    if (m_measurementHistory.size() > 1000) {
        m_measurementHistory.erase(m_measurementHistory.begin(), m_measurementHistory.begin() + 100);
    }
    
    m_lastUpdate = QDateTime::currentDateTime();
}

void WebServerAlternative::setCurrentConfig(const PHCConfig& config)
{
    QMutexLocker locker(&m_dataMutex);
    m_currentConfig = config;
}

void WebServerAlternative::setAvailableDevices(const std::vector<int>& devices)
{
    QMutexLocker locker(&m_dataMutex);
    m_availableDevices = devices;
}

void WebServerAlternative::setMeasurementStatus(bool measuring)
{
    QMutexLocker locker(&m_dataMutex);
    m_measuring = measuring;
}

void WebServerAlternative::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* client = m_tcpServer->nextPendingConnection();
        m_clients.append(client);
        
        connect(client, &QTcpSocket::disconnected, this, &WebServerAlternative::onClientDisconnected);
        connect(client, &QTcpSocket::readyRead, this, &WebServerAlternative::onClientData);
        
        qDebug() << "New client connected:" << client->peerAddress().toString();
    }
}

void WebServerAlternative::onClientDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        m_clients.removeAll(client);
        client->deleteLater();
        qDebug() << "Client disconnected:" << client->peerAddress().toString();
    }
}

void WebServerAlternative::onClientData()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    QByteArray data = client->readAll();
    QString request = QString::fromUtf8(data);
    
    handleRequest(client, request);
}

void WebServerAlternative::handleRequest(QTcpSocket* client, const QString& request)
{
    QString path = parsePath(request);
    
    if (path == "/" || path.isEmpty()) {
        // Serve main web interface
        QString html = loadWebInterface();
        sendResponse(client, "text/html; charset=utf-8", html.toUtf8());
    }
    else if (path == "/api/status") {
        QJsonObject status = getCurrentStatus();
        sendJsonResponse(client, status);
    }
    else if (path == "/api/history") {
        QJsonObject response;
        response["history"] = getMeasurementHistory();
        sendJsonResponse(client, response);
    }
    else if (path == "/api/devices") {
        QMutexLocker locker(&m_dataMutex);
        QJsonArray devices;
        for (int device : m_availableDevices) {
            devices.append(device);
        }
        QJsonObject response;
        response["devices"] = devices;
        sendJsonResponse(client, response);
    }
    else if (path == "/api/start") {
        emit measurementRequested(m_currentConfig);
        QJsonObject response;
        response["status"] = "started";
        sendJsonResponse(client, response);
    }
    else if (path == "/api/stop") {
        emit measurementStopped();
        QJsonObject response;
        response["status"] = "stopped";
        sendJsonResponse(client, response);
    }
    else if (path == "/api/refresh") {
        emit deviceRefreshRequested();
        QJsonObject response;
        response["status"] = "refreshed";
        sendJsonResponse(client, response);
    }
    else {
        // 404 Not Found
        QString html = "<html><body><h1>404 Not Found</h1><p>Page not found: " + path + "</p></body></html>";
        sendResponse(client, "text/html; charset=utf-8", html.toUtf8());
    }
}

void WebServerAlternative::sendResponse(QTcpSocket* client, const QString& contentType, const QByteArray& data)
{
    QString response = QString("HTTP/1.1 200 OK\r\n"
                              "Content-Type: %1\r\n"
                              "Content-Length: %2\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                              "Access-Control-Allow-Headers: Content-Type\r\n"
                              "Connection: close\r\n"
                              "\r\n").arg(contentType).arg(data.size());
    
    client->write(response.toUtf8());
    client->write(data);
    client->flush();
    client->disconnectFromHost();
}

void WebServerAlternative::sendJsonResponse(QTcpSocket* client, const QJsonObject& json)
{
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    sendResponse(client, "application/json; charset=utf-8", data);
}

QString WebServerAlternative::loadWebInterface()
{
    QFile file("web_interface.html");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open web_interface.html file";
        return QString("<html><body><h1>Error: Cannot load web interface</h1></body></html>");
    }
    
    QTextStream in(&file);
    return in.readAll();
}

QString WebServerAlternative::parsePath(const QString& request)
{
    QRegularExpression regex("^GET\\s+(\\S+)\\s+HTTP");
    QRegularExpressionMatch match = regex.match(request);
    
    if (match.hasMatch()) {
        return match.captured(1);
    }
    
    return "/";
}

QJsonObject WebServerAlternative::getCurrentStatus()
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

QJsonArray WebServerAlternative::getMeasurementHistory()
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

void WebServerAlternative::broadcastUpdate()
{
    // This would be used for WebSocket real-time updates
    // For now, we rely on client-side polling
}

#include "web_server_alternative.moc"
