#include "MainLogging.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QString>
#include <QSysInfo>

#include <cstdio>

struct RuntimeLogConfig
{
    QString level = "info";
    QString dir = "./logs";
    bool toConsole = true;
    int maxFiles = 14;
    QString env = "local";
    QString serviceName = "MemoChatQml";
    QString serviceInstance = "MemoChatQml@localhost";
};

RuntimeLogConfig g_log_cfg;
QMutex g_log_mutex;

int levelWeight(const QString& level)
{
    const QString v = level.trimmed().toLower();
    if (v == "debug")
    {
        return 0;
    }
    if (v == "info")
    {
        return 1;
    }
    if (v == "warn" || v == "warning")
    {
        return 2;
    }
    return 3;
}

int msgWeight(QtMsgType type)
{
    switch (type)
    {
        case QtDebugMsg:
            return 0;
        case QtInfoMsg:
            return 1;
        case QtWarningMsg:
            return 2;
        case QtCriticalMsg:
            return 3;
        case QtFatalMsg:
            return 3;
    }
    return 1;
}

QString msgLevel(QtMsgType type)
{
    switch (type)
    {
        case QtDebugMsg:
            return "debug";
        case QtInfoMsg:
            return "info";
        case QtWarningMsg:
            return "warn";
        case QtCriticalMsg:
            return "error";
        case QtFatalMsg:
            return "fatal";
    }
    return "info";
}

QString resolveLogDir(const QString& appPath, const QString& configuredDir)
{
    const QString dir = configuredDir.trimmed();
    if (dir.isEmpty())
    {
        return QDir(appPath).filePath("logs");
    }
    QDir maybeRelative(dir);
    if (maybeRelative.isRelative())
    {
        return QDir(appPath).filePath(dir);
    }
    return dir;
}

void cleanupOldLogs()
{
    QDir dir(g_log_cfg.dir);
    const QStringList files =
        dir.entryList(QStringList() << "MemoChatQml_*.json", QDir::Files, QDir::Name | QDir::Reversed);
    for (int i = g_log_cfg.maxFiles; i < files.size(); ++i)
    {
        dir.remove(files.at(i));
    }
}

void loadRuntimeLogConfig(const QString& configPath, const QString& appPath)
{
    QSettings settings(configPath, QSettings::IniFormat);
    g_log_cfg.level = settings.value("Log/Level", "info").toString().trimmed().toLower();
    g_log_cfg.dir = resolveLogDir(appPath, settings.value("Log/Dir", "./logs").toString());
    g_log_cfg.toConsole = settings.value("Log/ToConsole", true).toBool();
    g_log_cfg.maxFiles = settings.value("Log/MaxFiles", 14).toInt();
    if (g_log_cfg.maxFiles <= 0)
    {
        g_log_cfg.maxFiles = 14;
    }
    g_log_cfg.env = settings.value("Log/Env", "local").toString().trimmed();
    g_log_cfg.serviceName = settings.value("Telemetry/ServiceName", "MemoChatQml").toString().trimmed();
    if (g_log_cfg.serviceName.isEmpty())
    {
        g_log_cfg.serviceName = "MemoChatQml";
    }
    g_log_cfg.serviceInstance =
        QStringLiteral("%1@%2:%3") .arg(g_log_cfg.serviceName,
                                        QSysInfo::machineHostName().isEmpty()
                                        ? QStringLiteral("localhost") : QSysInfo::machineHostName(),
                                                         QString::number(QCoreApplication::applicationPid()));
    QDir().mkpath(g_log_cfg.dir);
}

void fileMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    QMutexLocker locker(&g_log_mutex);
    if (msgWeight(type) < levelWeight(g_log_cfg.level))
    {
        return;
    }

    cleanupOldLogs();
    const QString dateTag = QDate::currentDate().toString("yyyyMMdd");
    QFile file(QDir(g_log_cfg.dir).filePath(QString("MemoChatQml_%1.json").arg(dateTag)));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QJsonObject obj;
        obj["ts"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        obj["level"] = msgLevel(type);
        obj["service"] = g_log_cfg.serviceName;
        obj["service_instance"] = g_log_cfg.serviceInstance;
        obj["env"] = g_log_cfg.env;
        obj["event"] = "qt.message";
        obj["message"] = msg;
        obj["request_id"] = QString();
        obj["span_id"] = QString();
        obj["module"] = "qt";
        obj["peer_service"] = QString();
        obj["error_code"] = QString();
        obj["error_type"] = (type == QtCriticalMsg || type == QtFatalMsg) ? "qt" : QString();
        obj["duration_ms"] = 0;
        obj["attrs"] = QJsonObject();
        const QByteArray line = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        file.write(line);
        file.write("\n");
        file.flush();
        if (g_log_cfg.toConsole)
        {
            std::fprintf(stderr, "%s\n", line.constData());
        }
    }

    if (type == QtFatalMsg)
    {
        abort();
    }
}
