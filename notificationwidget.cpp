#include "notificationwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsOpacityEffect>

NotificationWidget::NotificationWidget(QWidget *parent)
    : QWidget(parent)
    , titleLabel(nullptr)
    , messageLabel(nullptr)
    , closeButton(nullptr)
    , closeTimer(new QTimer(this))
    , fadeOutAnimation(nullptr)
{
    setupUI();
    
    // 使用 lambda 确保连接安全
    connect(closeTimer, &QTimer::timeout, this, [this]() {
        hideNotification();
    });
    closeTimer->setSingleShot(true);
    
    // 连接关闭按钮
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        onCloseButtonClicked();
    });
}

NotificationWidget::~NotificationWidget()
{
    // 停止并清理定时器
    if (closeTimer) {
        closeTimer->stop();
    }
    
    // 停止并清理动画
    if (fadeOutAnimation) {
        fadeOutAnimation->stop();
        delete fadeOutAnimation;
        fadeOutAnimation = nullptr;
    }
}

void NotificationWidget::setupUI()
{
    // 设置窗口属性 - 无边框、置顶、工具窗口
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    
    setFixedSize(450, 150);
    
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 创建一个带背景的容器
    QWidget *container = new QWidget();
    container->setStyleSheet(
        "QWidget {"
        "    background-color: #2E3440;"
        "    border-radius: 12px;"
        "    border: 3px solid #88C0D0;"
        "}"
    );
    
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(8);
    containerLayout->setContentsMargins(20, 20, 20, 20);
    
    // 标题行 - 包含标题和关闭按钮
    QHBoxLayout *titleLayout = new QHBoxLayout();
    
    // 标题标签
    titleLabel = new QLabel();
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #88C0D0;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "    border: none;"
        "}"
    );
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    
    // 关闭按钮
    closeButton = new QPushButton("×");
    closeButton->setFixedSize(30, 30);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    color: #D8DEE9;"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "    border: none;"
        "    border-radius: 15px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #BF616A;"
        "    color: white;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #A54650;"
        "}"
    );
    closeButton->setCursor(Qt::PointingHandCursor);
    titleLayout->addWidget(closeButton);
    
    containerLayout->addLayout(titleLayout);
    
    // 消息标签
    messageLabel = new QLabel();
    messageLabel->setStyleSheet(
        "QLabel {"
        "    color: #ECEFF4;"
        "    font-size: 16px;"
        "    background: transparent;"
        "    border: none;"
        "}"
    );
    messageLabel->setWordWrap(true);
    containerLayout->addWidget(messageLabel);
    
    mainLayout->addWidget(container);
}

void NotificationWidget::showNotification(const QString &title, const QString &message, int duration)
{
    // 如果有正在进行的淡出动画，停止它
    if (fadeOutAnimation && fadeOutAnimation->state() == QAbstractAnimation::Running) {
        fadeOutAnimation->stop();
        delete fadeOutAnimation;
        fadeOutAnimation = nullptr;
    }
    
    // 停止之前的定时器
    if (closeTimer->isActive()) {
        closeTimer->stop();
    }
    
    titleLabel->setText(title);
    messageLabel->setText(message);
    
    // 重置透明度
    setWindowOpacity(1.0);
    
    // 显示窗口
    show();
    raise();
    activateWindow();
    
    // 设置定时器在指定时间后关闭
    closeTimer->start(duration);
}

void NotificationWidget::hideNotification()
{
    // 如果已有动画在运行，先停止它
    if (fadeOutAnimation) {
        fadeOutAnimation->stop();
        delete fadeOutAnimation;
        fadeOutAnimation = nullptr;
    }
    
    // 创建淡出动画
    fadeOutAnimation = new QPropertyAnimation(this, "windowOpacity");
    fadeOutAnimation->setDuration(500);
    fadeOutAnimation->setStartValue(1.0);
    fadeOutAnimation->setEndValue(0.0);
    
    // 使用 Qt::QueuedConnection 确保安全
    connect(fadeOutAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (this) {  // 安全检查
            hide();
            // 发送关闭信号
            emit notificationClosed();
            // 动画完成后清理指针
            if (fadeOutAnimation) {
                fadeOutAnimation->deleteLater();
                fadeOutAnimation = nullptr;
            }
        }
    }, Qt::QueuedConnection);
    
    fadeOutAnimation->start();
}

void NotificationWidget::onCloseButtonClicked()
{
    // 停止自动关闭定时器
    if (closeTimer->isActive()) {
        closeTimer->stop();
    }
    // 立即关闭
    hideNotification();
}
