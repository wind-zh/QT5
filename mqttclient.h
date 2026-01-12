#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <QtMqtt/QMqttClient>
#include <QJsonObject>
#include <QTimer>

class MqttClient : public QObject
{
    Q_OBJECT

public:
    explicit MqttClient(QObject *parent = nullptr);
    ~MqttClient();
    
    void connectToHost(const QString &host, quint16 port);
    void disconnectFromHost();
    void subscribe(const QString &topic);
    void unsubscribe(const QString &topic);
    
    bool isConnected() const;
    
    // 设置重连参数
    void setReconnectInterval(int intervalMs);
    void setMaxReconnectAttempts(int maxAttempts); // 0 表示无限重连

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void messageReceived(const QString &topic, const QByteArray &message);
    void reconnecting(int attemptCount);
    void doorEventReceived(const QJsonObject &eventData); // 门禁事件信号

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorChanged(QMqttClient::ClientError error);
    void onStateChanged(QMqttClient::ClientState state);
    void attemptReconnect();
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);

private:
    QMqttClient *m_client;
    QTimer *m_reconnectTimer;
    QMqttSubscription *m_subscription;
    
    QString m_host;
    quint16 m_port;
    QString m_subscribeTopic; // 订阅的主题
    bool m_autoReconnect;
    bool m_manualDisconnect; // 标记是否为手动断开
    int m_reconnectInterval;
    int m_maxReconnectAttempts;
    int m_currentReconnectAttempt;
};

#endif // MQTTCLIENT_H
