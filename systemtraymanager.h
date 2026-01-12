#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class ClientManager;

class SystemTrayManager : public QObject
{
    Q_OBJECT
    
public:
    explicit SystemTrayManager(ClientManager *manager, QObject *parent = nullptr);
    ~SystemTrayManager();
    
    void show();
    void setAutoStart(bool enable);
    bool isAutoStartEnabled();
    
private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowStatus();
    void onToggleAutoStart();
    void onExit();
    
private:
    void createTrayIcon();
    void createActions();
    void createMenu();
    void updateAutoStartAction();
    bool addToStartup();
    bool removeFromStartup();
    QString getStartupRegistryPath();
    
    ClientManager *m_clientManager;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    
    QAction *m_statusAction;
    QAction *m_autoStartAction;
    QAction *m_exitAction;
};

#endif // SYSTEMTRAYMANAGER_H
