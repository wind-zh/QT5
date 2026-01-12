#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <QObject>
#include <QSoundEffect>
#include "mqttclient.h"
#include "notificationwidget.h"

class ClientManager : public QObject
{
    Q_OBJECT

public:
    explicit ClientManager(QObject *parent = nullptr);
    ~ClientManager();
    
    void start();
    void stop();

private slots:
    void onMqttConnected();
    void onMqttDisconnected();
    void onMqttError(const QString &error);
    void onMqttReconnecting(int attemptCount);
    void onDoorEvent(const QJsonObject &eventData);

private:
    void playNotificationSound(const QString &soundPath, qreal volume = 1.0, const QString &loopMode = "once");
    
    MqttClient *mqttClient;
    NotificationWidget *notification;
    QSoundEffect *soundEffect;
};

#endif // CLIENTMANAGER_H
