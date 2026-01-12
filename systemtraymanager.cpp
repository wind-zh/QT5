#include "systemtraymanager.h"
#include "clientmanager.h"
#include "logger.h"
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QStyle>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SystemTrayManager::SystemTrayManager(ClientManager *manager, QObject *parent)
    : QObject(parent)
    , m_clientManager(manager)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
{
    createActions();
    createMenu();
    createTrayIcon();
    
    LOG_INFO("系统托盘管理器已初始化");
}

SystemTrayManager::~SystemTrayManager()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
        delete m_trayIcon;
    }
    if (m_trayMenu) {
        delete m_trayMenu;
    }
}

void SystemTrayManager::createActions()
{
    m_statusAction = new QAction(tr("查看状态"), this);
    connect(m_statusAction, &QAction::triggered, this, [this]() {
        onShowStatus();
    });
    
    m_autoStartAction = new QAction(tr("开机自启动"), this);
    m_autoStartAction->setCheckable(true);
    connect(m_autoStartAction, &QAction::triggered, this, [this]() {
        onToggleAutoStart();
    });
    
    m_exitAction = new QAction(tr("退出"), this);
    connect(m_exitAction, &QAction::triggered, this, [this]() {
        onExit();
    });
    
    updateAutoStartAction();
}

void SystemTrayManager::createMenu()
{
    m_trayMenu = new QMenu();
    m_trayMenu->addAction(m_statusAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_autoStartAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_exitAction);
}

void SystemTrayManager::createTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    
    // 设置图标 - 优先使用自定义图标，如果没有则使用系统默认图标
    QIcon icon(":/icons/app_icon.png");
    if (icon.isNull()) {
        // 如果自定义图标不存在，使用系统默认图标
        icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
        LOG_WARNING("未找到自定义图标，使用系统默认图标");
    } else {
        LOG_INFO("已加载自定义托盘图标");
    }
    m_trayIcon->setIcon(icon);
    
    m_trayIcon->setToolTip(tr("门禁状态客户端 - 运行中"));
    m_trayIcon->setContextMenu(m_trayMenu);
    
    // 使用 lambda 适配器处理带参数的信号
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
        onTrayIconActivated(reason);
    });
}

void SystemTrayManager::show()
{
    if (m_trayIcon) {
        m_trayIcon->show();
        m_trayIcon->showMessage(
            tr("门禁状态客户端"),
            tr("程序已在后台运行"),
            QSystemTrayIcon::Information,
            2000
        );
    }
}

void SystemTrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::DoubleClick:
        onShowStatus();
        break;
    default:
        break;
    }
}

void SystemTrayManager::onShowStatus()
{
    QString statusText = tr("门禁状态客户端\n\n");
    statusText += tr("状态: 运行中\n");
    statusText += tr("版本: %1\n").arg(QApplication::applicationVersion());
    statusText += tr("开机自启: %1\n").arg(isAutoStartEnabled() ? tr("已启用") : tr("未启用"));
    
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("状态信息"));
    msgBox.setText(statusText);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

void SystemTrayManager::onToggleAutoStart()
{
    bool enable = m_autoStartAction->isChecked();
    setAutoStart(enable);
    updateAutoStartAction();
}

void SystemTrayManager::onExit()
{
    LOG_INFO("用户通过托盘菜单退出程序");
    
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr,
                                  tr("确认退出"),
                                  tr("确定要退出门禁状态客户端吗？"),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (m_trayIcon) {
            m_trayIcon->hide();
        }
        QApplication::quit();
    } else {
        // 用户取消，恢复复选框状态
        if (m_autoStartAction->isCheckable()) {
            updateAutoStartAction();
        }
    }
}

QString SystemTrayManager::getStartupRegistryPath()
{
#ifdef Q_OS_WIN
    return "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
#else
    return QString();
#endif
}

bool SystemTrayManager::addToStartup()
{
#ifdef Q_OS_WIN
    QSettings settings(getStartupRegistryPath(), QSettings::NativeFormat);
    QString appPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
    
    // 添加引号以处理包含空格的路径
    appPath = "\"" + appPath + "\"";
    
    settings.setValue(QApplication::applicationName(), appPath);
    settings.sync();
    
    bool success = (settings.status() == QSettings::NoError);
    if (success) {
        LOG_INFO(QString("已添加到开机自启动: %1").arg(appPath));
    } else {
        LOG_ERROR("添加开机自启动失败");
    }
    return success;
#else
    return false;
#endif
}

bool SystemTrayManager::removeFromStartup()
{
#ifdef Q_OS_WIN
    QSettings settings(getStartupRegistryPath(), QSettings::NativeFormat);
    settings.remove(QApplication::applicationName());
    settings.sync();
    
    bool success = (settings.status() == QSettings::NoError);
    if (success) {
        LOG_INFO("已从开机自启动中移除");
    } else {
        LOG_ERROR("移除开机自启动失败");
    }
    return success;
#else
    return false;
#endif
}

void SystemTrayManager::setAutoStart(bool enable)
{
    bool success = false;
    if (enable) {
        success = addToStartup();
        if (success) {
            m_trayIcon->showMessage(
                tr("开机自启动"),
                tr("已启用开机自启动"),
                QSystemTrayIcon::Information,
                2000
            );
        }
    } else {
        success = removeFromStartup();
        if (success) {
            m_trayIcon->showMessage(
                tr("开机自启动"),
                tr("已禁用开机自启动"),
                QSystemTrayIcon::Information,
                2000
            );
        }
    }
    
    if (!success) {
        QMessageBox::warning(nullptr,
                           tr("操作失败"),
                           tr("设置开机自启动失败，请检查权限"));
    }
}

bool SystemTrayManager::isAutoStartEnabled()
{
#ifdef Q_OS_WIN
    QSettings settings(getStartupRegistryPath(), QSettings::NativeFormat);
    return settings.contains(QApplication::applicationName());
#else
    return false;
#endif
}

void SystemTrayManager::updateAutoStartAction()
{
    if (m_autoStartAction) {
        m_autoStartAction->setChecked(isAutoStartEnabled());
    }
}
