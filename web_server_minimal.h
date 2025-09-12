#ifndef WEB_SERVER_MINIMAL_H
#define WEB_SERVER_MINIMAL_H

#include <QObject>
#include <QHttpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QMutex>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <vector>
#include "diffphc_core.h"

class WebServerMinimal : public QObject
{
    Q_OBJECT

public:
    explicit WebServerMinimal(QObject *parent = nullptr);
    ~WebServerMinimal();
    
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
    QString loadWebInterface();
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
};

#endif // WEB_SERVER_MINIMAL_H
