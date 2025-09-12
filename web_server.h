#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <QObject>
#include <QHttpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <vector>
#include "diffphc_core.h"

class WebServer : public QObject
{
    Q_OBJECT

public:
    explicit WebServer(QObject *parent = nullptr);
    ~WebServer();
    
    bool startServer(quint16 port = 8080);
    void stopServer();
    bool isRunning() const;
    quint16 getPort() const;
    
    // Data management
    void addMeasurementResult(const PHCResult& result);
    void setCurrentConfig(const PHCConfig& config);
    void setAvailableDevices(const std::vector<int>& devices);
    void setMeasurementStatus(bool measuring);
    
signals:
    void measurementRequested(const PHCConfig& config);
    void measurementStopped();
    void configChanged(const PHCConfig& config);
    void deviceRefreshRequested();

private slots:
    void broadcastUpdate();

private:
    void setupRoutes();
    QByteArray createResponse(const QString& content, const QString& contentType = "text/html");
    QByteArray createJsonResponse(const QJsonObject& json);
    QByteArray createCorsHeaders();
    QString generateWebInterface();
    QString generateApiResponse(const QString& endpoint);
    QJsonObject getCurrentStatus();
    QJsonArray getMeasurementHistory();
    
    QHttpServer* m_httpServer;
    QTimer* m_broadcastTimer;
    QMutex m_dataMutex;
    
    // Server state
    bool m_running;
    quint16 m_port;
    
    // Data storage
    std::vector<PHCResult> m_measurementHistory;
    PHCConfig m_currentConfig;
    std::vector<int> m_availableDevices;
    bool m_measuring;
    QDateTime m_lastUpdate;
    
    // WebSocket clients for real-time updates (simplified for now)
    // QList<QTcpSocket*> m_webSocketClients;
};

#endif // WEB_SERVER_H
