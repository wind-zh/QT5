QT       += core gui network mqtt multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 应用程序名称
TARGET = DoorStateClient

# 模板类型
TEMPLATE = app

SOURCES += \
    main.cpp \
    clientmanager.cpp \
    mqttclient.cpp \
    notificationwidget.cpp \
    configmanager.cpp \
    logger.cpp \
    systemtraymanager.cpp

HEADERS += \
    clientmanager.h \
    mqttclient.h \
    notificationwidget.h \
    configmanager.h \
    logger.h \
    systemtraymanager.h

# 资源文件
RESOURCES += resources.qrc

# Windows 特定配置
win32 {
    # 设置为Windows GUI应用（无控制台窗口，后台运行）
    CONFIG -= console
    CONFIG += windows
    
    # 设置Windows程序图标（可选）
    RC_ICONS = app_icon.ico
    
    # 如果需要调试，可以临时启用 console 模式
    # CONFIG += console
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
