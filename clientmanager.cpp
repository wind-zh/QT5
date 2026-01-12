#include "clientmanager.h"
#include "configmanager.h"
#include "logger.h"
#include <QApplication>
#include <QScreen>
#include <QDateTime>
#include <QUrl>
#include <QFileInfo>

ClientManager::ClientManager(QObject *parent)
    : QObject(parent)
    , mqttClient(nullptr)
    , notification(nullptr)
    , soundEffect(nullptr)
{
    mqttClient = new MqttClient(this);
    notification = new NotificationWidget();
    soundEffect = new QSoundEffect(this);
    
    // 连接通知窗口关闭信号
    connect(notification, &NotificationWidget::notificationClosed, this, [this]() {
        // 通知关闭时停止音频
        if (soundEffect && soundEffect->isPlaying()) {
            soundEffect->stop();
            LOG_INFO("通知关闭，停止音频播放");
        }
    });
    
    // 使用 lambda 表达式确保信号槽连接安全
    connect(mqttClient, &MqttClient::connected, this, [this]() {
        onMqttConnected();
    });
    connect(mqttClient, &MqttClient::disconnected, this, [this]() {
        onMqttDisconnected();
    });
    connect(mqttClient, &MqttClient::errorOccurred, this, [this](const QString &error) {
        onMqttError(error);
    });
    connect(mqttClient, &MqttClient::reconnecting, this, [this](int attemptCount) {
        onMqttReconnecting(attemptCount);
    });
    connect(mqttClient, &MqttClient::doorEventReceived, this, [this](const QJsonObject &eventData) {
        onDoorEvent(eventData);
    });
}

ClientManager::~ClientManager()
{
    stop();
    if (notification) {
        delete notification;
        notification = nullptr;
    }
    if (soundEffect) {
        soundEffect->stop();
        delete soundEffect;
        soundEffect = nullptr;
    }
}

void ClientManager::start()
{
    ConfigManager *config = ConfigManager::instance();
    
    // 连接 MQTT 服务器
    QString mqttHost = config->getMqttHost();
    quint16 mqttPort = config->getMqttPort();
    mqttClient->connectToHost(mqttHost, mqttPort);
}

void ClientManager::stop()
{
    if (mqttClient) {
        mqttClient->disconnectFromHost();
    }
}

void ClientManager::onMqttConnected()
{
    LOG_INFO("MQTT 客户端连接成功");
    
    // 连接成功后订阅控制主题
    ConfigManager *config = ConfigManager::instance();
    QString subscribeTopic = config->getMqttSubscribeTopic();
    mqttClient->subscribe(subscribeTopic);
}

void ClientManager::onMqttDisconnected()
{
    LOG_WARNING("MQTT 客户端断开连接");
}

void ClientManager::onMqttError(const QString &error)
{
    LOG_ERROR(QString("MQTT 连接错误: %1").arg(error));
}

void ClientManager::onMqttReconnecting(int attemptCount)
{
    LOG_INFO(QString("MQTT 正在尝试第 %1 次重连...").arg(attemptCount));
}

void ClientManager::onDoorEvent(const QJsonObject &eventData)
{
    LOG_INFO("收到门禁事件");
    
    // 获取时间戳 - 优先使用服务端发送的 timestamp，没有则使用客户端当前时间
    QString timeStr;
    if (eventData.contains("timestamp")) {
        // 解析服务端的 ISO 格式时间戳
        QString timestamp = eventData["timestamp"].toString();
        QDateTime dateTime = QDateTime::fromString(timestamp, Qt::ISODate);
        if (dateTime.isValid()) {
            timeStr = dateTime.toString("HH:mm:ss");
        } else {
            // 如果解析失败，使用客户端当前时间
            timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
        }
    } else {
        // 没有 timestamp 字段，使用客户端当前时间
        timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    }
    
    QString title = QString("门禁通知 - %1").arg(timeStr);
    QString message = "开门按钮已被按下!";
    
    // 检查是否有自定义的事件类型或消息
    if (eventData.contains("event")) {
        QString eventType = eventData["event"].toString();
        if (eventType == "door_button_pressed") {
            message = "开门按钮已被按下!";
        } else if (eventType == "door_button_released") {
            message = "开门按钮已松开";
        } else {
            message = QString("门禁事件: %1").arg(eventType);
        }
    }
    
    // 如果包含自定义消息
    if (eventData.contains("message")) {
        message = eventData["message"].toString();
    }
    
    // 从配置文件获取通知显示时长
    ConfigManager *config = ConfigManager::instance();
    int duration = config->getNotificationDuration();
    
    // 播放通知音频
    QString soundPath = config->getNotificationSoundPath();
    qreal soundVolume = config->getNotificationSoundVolume();
    QString soundLoop = config->getNotificationSoundLoop();
    
    if (!soundPath.isEmpty()) {
        playNotificationSound(soundPath, soundVolume, soundLoop);
    }
    
    // 显示通知窗口在屏幕右下角
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        
        int x = screenGeometry.right() - notification->width() - 20;
        int y = screenGeometry.bottom() - notification->height() - 20;
        
        notification->move(x, y);
        notification->showNotification(title, message, duration);
        
        LOG_INFO(QString("显示通知: %1 - %2").arg(title).arg(message));
    }
}

void ClientManager::playNotificationSound(const QString &soundPath, qreal volume, const QString &loopMode)
{
    if (!soundEffect) {
        LOG_WARNING("音频播放器未初始化");
        return;
    }
    
    // 停止之前的播放
    if (soundEffect->isPlaying()) {
        soundEffect->stop();
    }
    
    // 检查文件是否存在
    QFileInfo fileInfo(soundPath);
    if (!fileInfo.exists()) {
        LOG_WARNING(QString("音频文件不存在: %1").arg(soundPath));
        return;
    }
    
    // 确保音量在有效范围内
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.0) volume = 1.0;
    
    // 设置音频源
    QUrl soundUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
    soundEffect->setSource(soundUrl);
    soundEffect->setVolume(volume);
    
    // 设置循环次数
    if (loopMode == "loop") {
        soundEffect->setLoopCount(QSoundEffect::Infinite);  // 无限循环
        LOG_INFO(QString("播放通知音频（循环模式）: %1 (音量: %2)").arg(soundPath).arg(volume));
    } else {
        soundEffect->setLoopCount(1);  // 播放一次
        LOG_INFO(QString("播放通知音频（单次模式）: %1 (音量: %2)").arg(soundPath).arg(volume));
    }
    
    // 播放音频
    soundEffect->play();
}
