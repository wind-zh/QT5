#include "mqttclient.h"
#include "logger.h"
#include <QJsonDocument>
#include <QtMqtt/QMqttMessage>

MqttClient::MqttClient(QObject *parent)
    : QObject(parent)
    , m_client(nullptr)
    , m_reconnectTimer(nullptr)
    , m_subscription(nullptr)
    , m_port(1883)
    , m_autoReconnect(true)
    , m_manualDisconnect(false)
    , m_reconnectInterval(5000) // 默认 5 秒重连间隔
    , m_maxReconnectAttempts(0) // 默认无限重连
    , m_currentReconnectAttempt(0)
{
    m_client = new QMqttClient(this);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    
    // 使用新式信号槽语法
    connect(m_client, &QMqttClient::connected, this, &MqttClient::onConnected);
    connect(m_client, &QMqttClient::disconnected, this, &MqttClient::onDisconnected);
    
    // 使用 lambda 适配器处理带参数的信号
    connect(m_client, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError error) {
        onErrorChanged(error);
    });
    connect(m_client, &QMqttClient::stateChanged, this, [this](QMqttClient::ClientState state) {
        onStateChanged(state);
    });
    
    connect(m_reconnectTimer, &QTimer::timeout, this, &MqttClient::attemptReconnect);
}

MqttClient::~MqttClient()
{
    m_autoReconnect = false; // 禁用自动重连
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    if (m_client && m_client->state() == QMqttClient::Connected) {
        m_client->disconnectFromHost();
    }
}

void MqttClient::connectToHost(const QString &host, quint16 port)
{
    if (m_client->state() == QMqttClient::Connected) {
        LOG_WARNING("MQTT 客户端已连接");
        return;
    }
    
    m_host = host;
    m_port = port;
    m_manualDisconnect = false;
    m_currentReconnectAttempt = 0;
    
    m_client->setHostname(m_host);
    m_client->setPort(m_port);
    
    LOG_INFO(QString("正在连接到 MQTT 服务器 %1:%2...").arg(m_host).arg(m_port));
    m_client->connectToHost();
}

void MqttClient::disconnectFromHost()
{
    m_manualDisconnect = true; // 标记为手动断开
    m_autoReconnect = false; // 禁用自动重连
    
    if (m_reconnectTimer && m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }
    
    if (m_client->state() == QMqttClient::Connected) {
        LOG_INFO("断开 MQTT 连接");
        m_client->disconnectFromHost();
    }
}

bool MqttClient::isConnected() const
{
    return m_client && m_client->state() == QMqttClient::Connected;
}

void MqttClient::subscribe(const QString &topic)
{
    if (m_client->state() != QMqttClient::Connected) {
        LOG_WARNING(QString("MQTT 未连接，无法订阅主题: %1").arg(topic));
        m_subscribeTopic = topic; // 保存主题，连接后自动订阅
        return;
    }
    
    m_subscribeTopic = topic;
    m_subscription = m_client->subscribe(topic, 0);
    
    if (!m_subscription) {
        LOG_ERROR(QString("MQTT 订阅失败，主题: %1").arg(topic));
        return;
    }
    
    // 使用 lambda 表达式处理消息接收
    connect(m_subscription, &QMqttSubscription::messageReceived, 
            this, [this](const QMqttMessage &msg) {
        onMessageReceived(msg.payload(), msg.topic());
    });
    
    LOG_INFO(QString("MQTT 已订阅主题: %1").arg(topic));
}

void MqttClient::unsubscribe(const QString &topic)
{
    if (m_client->state() != QMqttClient::Connected) {
        LOG_WARNING("MQTT 未连接，无法取消订阅");
        return;
    }
    
    m_client->unsubscribe(topic);
    LOG_INFO(QString("MQTT 已取消订阅主题: %1").arg(topic));
}

void MqttClient::onConnected()
{
    m_currentReconnectAttempt = 0; // 重置重连计数
    m_autoReconnect = true; // 启用自动重连
    
    LOG_INFO("MQTT 客户端已连接");
    emit connected();
    
    // 如果有保存的订阅主题，自动订阅
    if (!m_subscribeTopic.isEmpty()) {
        subscribe(m_subscribeTopic);
    }
}

void MqttClient::onDisconnected()
{
    LOG_WARNING("MQTT 客户端已断开");
    emit disconnected();
    
    // 如果不是手动断开且启用了自动重连，则尝试重连
    if (!m_manualDisconnect && m_autoReconnect) {
        if (m_maxReconnectAttempts == 0 || m_currentReconnectAttempt < m_maxReconnectAttempts) {
            m_currentReconnectAttempt++;
            LOG_INFO(QString("将在 %1 秒后尝试第 %2 次重连...")
                     .arg(m_reconnectInterval / 1000)
                     .arg(m_currentReconnectAttempt));
            m_reconnectTimer->start(m_reconnectInterval);
        } else {
            LOG_ERROR(QString("已达到最大重连次数 (%1)，停止重连").arg(m_maxReconnectAttempts));
        }
    }
}

void MqttClient::onErrorChanged(QMqttClient::ClientError error)
{
    QString errorStr;
    switch (error) {
        case QMqttClient::NoError:
            return;
        case QMqttClient::InvalidProtocolVersion:
            errorStr = "无效的协议版本";
            break;
        case QMqttClient::IdRejected:
            errorStr = "ID 被拒绝";
            break;
        case QMqttClient::ServerUnavailable:
            errorStr = "服务器不可用";
            break;
        case QMqttClient::BadUsernameOrPassword:
            errorStr = "用户名或密码错误";
            break;
        case QMqttClient::NotAuthorized:
            errorStr = "未授权";
            break;
        case QMqttClient::TransportInvalid:
            errorStr = "传输无效";
            break;
        case QMqttClient::ProtocolViolation:
            errorStr = "协议违规";
            break;
        case QMqttClient::UnknownError:
        default:
            errorStr = "未知错误";
            break;
    }
    
    LOG_ERROR(QString("MQTT 错误: %1").arg(errorStr));
    emit errorOccurred(errorStr);
}

void MqttClient::onStateChanged(QMqttClient::ClientState state)
{
    QString stateStr;
    switch (state) {
        case QMqttClient::Disconnected:
            stateStr = "已断开";
            break;
        case QMqttClient::Connecting:
            stateStr = "正在连接";
            break;
        case QMqttClient::Connected:
            stateStr = "已连接";
            break;
        default:
            stateStr = "未知状态";
            break;
    }
    
    LOG_DEBUG(QString("MQTT 状态变化: %1").arg(stateStr));
}

void MqttClient::attemptReconnect()
{
    if (m_client->state() == QMqttClient::Connected) {
        LOG_INFO("MQTT 已连接，取消重连");
        return;
    }
    
    LOG_INFO(QString("正在尝试重连到 MQTT 服务器 %1:%2 (第 %3 次尝试)...")
             .arg(m_host)
             .arg(m_port)
             .arg(m_currentReconnectAttempt));
    
    emit reconnecting(m_currentReconnectAttempt);
    
    m_client->setHostname(m_host);
    m_client->setPort(m_port);
    m_client->connectToHost();
}

void MqttClient::setReconnectInterval(int intervalMs)
{
    m_reconnectInterval = intervalMs;
}

void MqttClient::setMaxReconnectAttempts(int maxAttempts)
{
    m_maxReconnectAttempts = maxAttempts;
}

void MqttClient::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString topicStr = topic.name();
    LOG_INFO(QString("MQTT 收到消息，主题: %1, 内容: %2")
             .arg(topicStr)
             .arg(QString::fromUtf8(message)));
    
    emit messageReceived(topicStr, message);
    
    // 解析 JSON 消息
    QJsonDocument doc = QJsonDocument::fromJson(message);
    if (!doc.isObject()) {
        LOG_WARNING("MQTT 消息不是有效的 JSON 对象");
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // 发送门禁事件信号
    emit doorEventReceived(obj);
}
