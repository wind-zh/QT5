#ifndef NOTIFICATIONWIDGET_H
#define NOTIFICATIONWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPushButton>

class NotificationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationWidget(QWidget *parent = nullptr);
    ~NotificationWidget();

    void showNotification(const QString &title, const QString &message, int duration = 3000);

signals:
    void notificationClosed();  // 通知窗口关闭信号（自动或手动）

private slots:
    void hideNotification();
    void onCloseButtonClicked();

private:
    void setupUI();

    QLabel *titleLabel;
    QLabel *messageLabel;
    QPushButton *closeButton;
    QTimer *closeTimer;
    QPropertyAnimation *fadeOutAnimation;
};

#endif // NOTIFICATIONWIDGET_H
