#include "logger.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>

Logger* Logger::m_instance = nullptr;

Logger* Logger::instance()
{
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

Logger::Logger(QObject *parent)
    : QObject(parent)
    , logFile(nullptr)
    , logStream(nullptr)
    , logPath(QDir::homePath() + "/logs")
    , retentionDays(7)
{
    currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    // 使用默认路径打开日志文件
    openLogFile();
}

Logger::~Logger()
{
    closeLogFile();
}

void Logger::setLogPath(const QString &path)
{
    QMutexLocker locker(&mutex);
    if (logPath != path) {
        closeLogFile();
        logPath = path;
        // 只有在路径非空时才打开日志文件
        if (!logPath.isEmpty()) {
            openLogFile();
        }
    }
}

void Logger::setRetentionDays(int days)
{
    retentionDays = days;
}

void Logger::log(const QString &message, const QString &level)
{
    QMutexLocker locker(&mutex);
    
    // 如果日志文件未初始化（路径为空），则只输出到控制台
    if (logPath.isEmpty()) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString logMessage = QString("[%1] [%2] %3").arg(timestamp).arg(level).arg(message);
        qDebug().noquote() << logMessage;
        return;
    }
    
    // 检查日期是否变化，如果变化则切换日志文件
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    if (today != currentDate) {
        currentDate = today;
        closeLogFile();
        openLogFile();
        cleanOldLogs();
    }
    
    if (!logStream) {
        qWarning() << "Log stream is not available";
        return;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString logMessage = QString("[%1] [%2] %3").arg(timestamp).arg(level).arg(message);
    
    // 写入文件
    *logStream << logMessage << "\n";
    logStream->flush();
    
    // 同时输出到控制台
    qDebug().noquote() << logMessage;
}

void Logger::openLogFile()
{
    // 创建日志目录
    QDir dir;
    if (!dir.exists(logPath)) {
        if (!dir.mkpath(logPath)) {
            qWarning() << "Failed to create log directory:" << logPath;
            return;
        }
    }
    
    // 打开日志文件
    QString fileName = getCurrentLogFileName();
    logFile = new QFile(fileName);
    
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << fileName;
        delete logFile;
        logFile = nullptr;
        return;
    }
    
    logStream = new QTextStream(logFile);
    logStream->setCodec("UTF-8");
    
    qDebug() << "Log file opened:" << fileName;
}

void Logger::closeLogFile()
{
    if (logStream) {
        delete logStream;
        logStream = nullptr;
    }
    
    if (logFile) {
        logFile->close();
        delete logFile;
        logFile = nullptr;
    }
}

void Logger::cleanOldLogs()
{
    if (retentionDays <= 0) {
        return;
    }
    
    QDir dir(logPath);
    QFileInfoList fileList = dir.entryInfoList(QStringList() << "DoorState_*.log", 
                                                QDir::Files, 
                                                QDir::Time | QDir::Reversed);
    
    QDate cutoffDate = QDate::currentDate().addDays(-retentionDays);
    
    for (const QFileInfo &fileInfo : fileList) {
        QString baseName = fileInfo.baseName();
        // 提取日期部分: DoorState_2025-12-16.log -> 2025-12-16
        QString dateStr = baseName.mid(10); // "DoorState_" 长度为 10
        QDate fileDate = QDate::fromString(dateStr, "yyyy-MM-dd");
        
        if (fileDate.isValid() && fileDate < cutoffDate) {
            if (dir.remove(fileInfo.fileName())) {
                qDebug() << "Removed old log file:" << fileInfo.fileName();
            }
        }
    }
}

QString Logger::getCurrentLogFileName() const
{
    QString fileName = QString("DoorState_%1.log").arg(currentDate);
    return QDir(logPath).filePath(fileName);
}
