#include <QApplication>
#include "clientmanager.h"
#include "logger.h"
#include "configmanager.h"
#include "systemtraymanager.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    QApplication::setApplicationName("DoorStateClient");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("DoorControl");
    
    // 检查系统托盘是否可用
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QObject::tr("系统托盘"),
                            QObject::tr("检测不到系统托盘！"));
        return 1;
    }
    
    // 防止应用在关闭最后一个窗口时退出（因为我们使用托盘）
    QApplication::setQuitOnLastWindowClosed(false);
    
    // 初始化配置管理器
    QString configPath = "config.ini";
    if (argc > 1) {
        configPath = QString::fromLocal8Bit(argv[1]);
    }
    ConfigManager::instance(configPath);
    
    // 初始化日志系统
    Logger *logger = Logger::instance();
    ConfigManager *config = ConfigManager::instance();
    logger->setLogPath(config->getLogPath());
    logger->setRetentionDays(config->getLogRetentionDays());
    
    LOG_INFO("========================================");
    LOG_INFO("DoorStateClient 启动");
    LOG_INFO(QString("弹窗显示时间: %1 ms").arg(config->getNotificationDuration()));
    LOG_INFO(QString("通知音量: %1").arg(config->getNotificationSoundVolume()));
    LOG_INFO(QString("通知音频路径: %1").arg(config->getNotificationSoundPath()));
    LOG_INFO(QString("通知音频循环模式: %1").arg(config->getNotificationSoundLoop()));
    LOG_INFO("========================================");
    
    // 创建并启动客户端管理器
    ClientManager manager;
    manager.start();
    
    // 创建并显示系统托盘
    SystemTrayManager trayManager(&manager);
    trayManager.show();
    
    LOG_INFO("客户端已启动，等待门禁事件...");
    LOG_INFO("程序运行在系统托盘中");
    
    return app.exec();
}
