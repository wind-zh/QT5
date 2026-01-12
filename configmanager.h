#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    static ConfigManager* instance(const QString &configPath = QString());
    
    QString getMqttHost() const;
    quint16 getMqttPort() const;
    QString getMqttSubscribeTopic() const;
    int getNotificationDuration() const;
    QString getNotificationSoundPath() const;
    qreal getNotificationSoundVolume() const;
    QString getNotificationSoundLoop() const;
    QString getLogPath() const;
    int getLogRetentionDays() const;
    
    void setMqttHost(const QString &host);
    void setMqttPort(quint16 port);
    void setMqttSubscribeTopic(const QString &topic);
    void setNotificationDuration(int duration);
    void setNotificationSoundPath(const QString &path);
    void setNotificationSoundVolume(qreal volume);
    void setNotificationSoundLoop(const QString &mode);
    void setLogPath(const QString &path);
    void setLogRetentionDays(int days);
    
private:
    explicit ConfigManager(const QString &configPath, QObject *parent = nullptr);
    void loadConfig();
    void saveConfig();
    
    static ConfigManager *m_instance;
    QSettings *settings;
    QString configFilePath;
};

#endif // CONFIGMANAGER_H
