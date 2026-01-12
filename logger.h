#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QMutex>

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger* instance();
    
    void setLogPath(const QString &path);
    void setRetentionDays(int days);
    void log(const QString &message, const QString &level = "INFO");
    
private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    
    void openLogFile();
    void closeLogFile();
    void cleanOldLogs();
    QString getCurrentLogFileName() const;
    
    static Logger *m_instance;
    QFile *logFile;
    QTextStream *logStream;
    QString logPath;
    int retentionDays;
    QMutex mutex;
    QString currentDate;
};

// 便捷宏
#define LOG_INFO(msg) Logger::instance()->log(msg, "INFO")
#define LOG_ERROR(msg) Logger::instance()->log(msg, "ERROR")
#define LOG_WARNING(msg) Logger::instance()->log(msg, "WARNING")
#define LOG_DEBUG(msg) Logger::instance()->log(msg, "DEBUG")

#endif // LOGGER_H
