#include "configmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

ConfigManager* ConfigManager::m_instance = nullptr;

ConfigManager* ConfigManager::instance(const QString &configPath)
{
    if (!m_instance) {
        m_instance = new ConfigManager(configPath);
    }
    return m_instance;
}

ConfigManager::ConfigManager(const QString &configPath, QObject *parent)
    : QObject(parent)
{
    // 如果没有指定配置文件路径，使用默认路径
    if (configPath.isEmpty()) {
        configFilePath = QCoreApplication::applicationDirPath() + "/config.ini";
    } else {
        // 处理相对路径和绝对路径
        QFileInfo fileInfo(configPath);
        if (fileInfo.isAbsolute()) {
            configFilePath = configPath;
        } else {
            configFilePath = QDir::current().absoluteFilePath(configPath);
        }
    }
    
    settings = new QSettings(configFilePath, QSettings::IniFormat);
    settings->setIniCodec("UTF-8");
    
    loadConfig();
    
    qDebug() << "Config file loaded from:" << configFilePath;
}

void ConfigManager::loadConfig()
{
    // 如果配置文件不存在或缺少配置项，使用默认值
    if (!settings->contains("MQTT/host")) {
        settings->setValue("MQTT/host", "localhost");
    }
    if (!settings->contains("MQTT/port")) {
        settings->setValue("MQTT/port", 1883);
    }
    if (!settings->contains("MQTT/subscribe_topic")) {
        settings->setValue("MQTT/subscribe_topic", "door-events");
    }
    if (!settings->contains("Notification/duration")) {
        settings->setValue("Notification/duration", 3000);
    }
    if (!settings->contains("Notification/sound_path")) {
        settings->setValue("Notification/sound_path", "");
    }
    if (!settings->contains("Notification/sound_volume")) {
        settings->setValue("Notification/sound_volume", 1.0);
    }
    if (!settings->contains("Notification/sound_loop")) {
        settings->setValue("Notification/sound_loop", "loop");
    }
    if (!settings->contains("Log/path")) {
        settings->setValue("Log/path", "./logs");
    }
    if (!settings->contains("Log/retention_days")) {
        settings->setValue("Log/retention_days", 7);
    }
    settings->sync();
}

void ConfigManager::saveConfig()
{
    settings->sync();
}

QString ConfigManager::getMqttHost() const
{
    return settings->value("MQTT/host", "localhost").toString();
}

quint16 ConfigManager::getMqttPort() const
{
    return settings->value("MQTT/port", 1883).toUInt();
}

QString ConfigManager::getMqttSubscribeTopic() const
{
    return settings->value("MQTT/subscribe_topic", "door-events").toString();
}

int ConfigManager::getNotificationDuration() const
{
    return settings->value("Notification/duration", 3000).toInt();
}

QString ConfigManager::getNotificationSoundPath() const
{
    return settings->value("Notification/sound_path", "").toString();
}

qreal ConfigManager::getNotificationSoundVolume() const
{
    qreal volume = settings->value("Notification/sound_volume", 1.0).toReal();
    // 确保音量在有效范围内 (0.0 - 1.0)
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.0) volume = 1.0;
    return volume;
}

QString ConfigManager::getNotificationSoundLoop() const
{
    QString mode = settings->value("Notification/sound_loop", "loop").toString().toLower();
    // 只接受 "once" 或 "loop"
    if (mode != "once" && mode != "loop") {
        mode = "loop";
    }
    return mode;
}

QString ConfigManager::getLogPath() const
{
    return settings->value("Log/path", "./logs").toString();
}

int ConfigManager::getLogRetentionDays() const
{
    return settings->value("Log/retention_days", 7).toInt();
}

void ConfigManager::setMqttHost(const QString &host)
{
    settings->setValue("MQTT/host", host);
    saveConfig();
}

void ConfigManager::setMqttPort(quint16 port)
{
    settings->setValue("MQTT/port", port);
    saveConfig();
}

void ConfigManager::setMqttSubscribeTopic(const QString &topic)
{
    settings->setValue("MQTT/subscribe_topic", topic);
    saveConfig();
}

void ConfigManager::setNotificationDuration(int duration)
{
    settings->setValue("Notification/duration", duration);
    saveConfig();
}

void ConfigManager::setNotificationSoundPath(const QString &path)
{
    settings->setValue("Notification/sound_path", path);
    saveConfig();
}

void ConfigManager::setNotificationSoundVolume(qreal volume)
{
    // 确保音量在有效范围内
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.0) volume = 1.0;
    settings->setValue("Notification/sound_volume", volume);
    saveConfig();
}

void ConfigManager::setNotificationSoundLoop(const QString &mode)
{
    QString validMode = mode.toLower();
    if (validMode != "once" && validMode != "loop") {
        validMode = "loop";
    }
    settings->setValue("Notification/sound_loop", validMode);
    saveConfig();
}

void ConfigManager::setLogPath(const QString &path)
{
    settings->setValue("Log/path", path);
    saveConfig();
}

void ConfigManager::setLogRetentionDays(int days)
{
    settings->setValue("Log/retention_days", days);
    saveConfig();
}
