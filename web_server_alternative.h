#ifndef WEB_SERVER_ALTERNATIVE_H
#define WEB_SERVER_ALTERNATIVE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QMutex>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <vector>
#include "diffphc_core.h"

class WebServerAlternative : public QObject
{
    Q_OBJECT

public:
    explicit WebServerAlternative(QObject *parent = nullptr);
    ~WebServerAlternative();
    
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
    void onNewConnection();
    void onClientDisconnected();
    void onClientData();
    void broadcastUpdate();

private:
    void handleRequest(QTcpSocket* client, const QString& request);
    void sendResponse(QTcpSocket* client, const QString& contentType, const QByteArray& data);
    void sendJsonResponse(QTcpSocket* client, const QJsonObject& json);
    QString loadWebInterface();
    QJsonObject getCurrentStatus();
    QJsonArray getMeasurementHistory();
    QString parsePath(const QString& request);
    
    QTcpServer* m_tcpServer;
    QTimer* m_broadcastTimer;
    QMutex m_dataMutex;
    QList<QTcpSocket*> m_clients;
    
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

#endif // WEB_SERVER_ALTERNATIVE_H
