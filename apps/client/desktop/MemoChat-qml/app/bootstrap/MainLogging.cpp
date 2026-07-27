#include "MainLogging.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
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
    bool redact = true;
    bool logDirPrivate = true;
    bool fileFailureReported = false;
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

#if defined(Q_OS_UNIX)
bool hasOwnerOnlyPermissions(const QFileInfo& info, QFileDevice::Permissions requiredPermissions)
{
    const QFileDevice::Permissions permissions = info.permissions();
    const QFileDevice::Permissions sharedPermissions = QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                                       QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                                       QFileDevice::WriteOther | QFileDevice::ExeOther;
    return !info.isSymLink() && (permissions & requiredPermissions) == requiredPermissions &&
           (permissions & sharedPermissions) == 0;
}

bool ensurePrivateLogDirectory(const QString& path)
{
    const QFileDevice::Permissions ownerDirectoryPermissions =
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner;
    QFileInfo info(path);
    if (info.exists())
    {
        return info.isDir() && hasOwnerOnlyPermissions(info, ownerDirectoryPermissions);
    }
    if (!QDir().mkpath(path) || !QFile::setPermissions(path, ownerDirectoryPermissions))
    {
        return false;
    }
    info.refresh();
    return info.isDir() && hasOwnerOnlyPermissions(info, ownerDirectoryPermissions);
}

bool openPrivateLogFile(QFile& file, QIODevice::OpenMode openMode)
{
    const QFileDevice::Permissions ownerFilePermissions = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
    QFileInfo info(file.fileName());
    if (info.exists())
    {
        return info.isFile() && hasOwnerOnlyPermissions(info, ownerFilePermissions) && file.open(openMode);
    }
    if (!file.open(openMode, ownerFilePermissions))
    {
        return false;
    }
    info.refresh();
    if (hasOwnerOnlyPermissions(info, ownerFilePermissions))
    {
        return true;
    }
    file.close();
    return false;
}
#else
bool ensurePrivateLogDirectory(const QString& path)
{
    return QDir().mkpath(path);
}

bool openPrivateLogFile(QFile& file, QIODevice::OpenMode openMode)
{
    return file.open(openMode);
}
#endif

QString replaceSensitiveMatch(const QString& message, const QRegularExpression& expression, const QString& replacement)
{
    QString result = message;
    result.replace(expression, replacement);
    return result;
}

QString redactSensitiveLogMessage(const QString& message)
{
    static const QString sensitiveKeyPattern = QStringLiteral(
        R"((?:access[_-]?(?:token|key)|refresh[_-]?token|auth[_-]?token|client[_-]?secret|jwt[_-]?(?:secret|key)|hmac[_-]?key|private[_-]?key|secret[_-]?key|turn[_-]?credential|login[_-]?ticket|verify[_-]?code|token|authorization|password|passwd|pwd|secret|session(?:[_-]?id)?|cookie|api[_-]?key|email))");
    static const QRegularExpression authorizationHeader(
        QStringLiteral(R"(((?:Proxy-)?Authorization\s*:\s*)(?:Bearer\s+|Basic\s+)?[^\s,;]+)"),
                       QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression cookieHeader(QStringLiteral(R"(((?:Set-)?Cookie\s*:\s*)[^\r\n]+)"),
                                                                QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression uriUserInfo(QStringLiteral(R"(([A-Z][A-Z0-9+.-]*://)[^/@\s]+@)"),
                                                               QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression queryParameter(QStringLiteral(R"(([?&]%1=)[^&#\s"']+)") .arg(sensitiveKeyPattern),
                                                                  QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression quotedAssignment(
        QStringLiteral(
            R"regex(((?:^|[\s,{])"?%1"?\s*[:=]\s*")([^"]*)("))regex") .arg(sensitiveKeyPattern),
                       QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression unquotedAssignment(
        QStringLiteral(
            R"(((?:^|[\s,{])%1\s*[:=]\s*)(?:Bearer\s+|Basic\s+)?[^\s,;}&]+)") .arg(sensitiveKeyPattern),
                       QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression emailAddress(QStringLiteral(R"(\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b)"),
                                                                QRegularExpression::CaseInsensitiveOption);

    QString redacted = replaceSensitiveMatch(message, authorizationHeader, QStringLiteral("\\1[REDACTED]"));
    redacted = replaceSensitiveMatch(redacted, cookieHeader, QStringLiteral("\\1[REDACTED]"));
    redacted = replaceSensitiveMatch(redacted, uriUserInfo, QStringLiteral("\\1[REDACTED]@"));
    redacted = replaceSensitiveMatch(redacted, queryParameter, QStringLiteral("\\1[REDACTED]"));
    redacted = replaceSensitiveMatch(redacted, quotedAssignment, QStringLiteral("\\1[REDACTED]\\3"));
    redacted = replaceSensitiveMatch(redacted, unquotedAssignment, QStringLiteral("\\1[REDACTED]"));
    return replaceSensitiveMatch(redacted, emailAddress, QStringLiteral("[REDACTED]"));
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
    g_log_cfg.redact = settings.value("Log/Redact", true).toBool();
    g_log_cfg.fileFailureReported = false;
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
    g_log_cfg.logDirPrivate = ensurePrivateLogDirectory(g_log_cfg.dir);
}

void fileMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    QMutexLocker locker(&g_log_mutex);
    if (msgWeight(type) < levelWeight(g_log_cfg.level))
    {
        return;
    }

    const QString safeMessage = g_log_cfg.redact ? redactSensitiveLogMessage(msg) : msg;
    QJsonObject obj;
    obj["ts"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    obj["level"] = msgLevel(type);
    obj["service"] = g_log_cfg.serviceName;
    obj["service_instance"] = g_log_cfg.serviceInstance;
    obj["env"] = g_log_cfg.env;
    obj["event"] = "qt.message";
    obj["message"] = safeMessage;
    obj["request_id"] = QString();
    obj["span_id"] = QString();
    obj["module"] = "qt";
    obj["peer_service"] = QString();
    obj["error_code"] = QString();
    obj["error_type"] = (type == QtCriticalMsg || type == QtFatalMsg) ? "qt" : QString();
    obj["duration_ms"] = 0;
    obj["attrs"] = QJsonObject();
    const QByteArray line = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    if (g_log_cfg.logDirPrivate)
    {
        cleanupOldLogs();
    }
    const QString dateTag = QDate::currentDate().toString("yyyyMMdd");
    QFile file(QDir(g_log_cfg.dir).filePath(QString("MemoChatQml_%1.json").arg(dateTag)));
    const QIODevice::OpenMode openMode = QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
    const bool fileOpened = g_log_cfg.logDirPrivate && openPrivateLogFile(file, openMode);
    if (fileOpened)
    {
        file.write(line);
        file.write("\n");
        file.flush();
    }
    else if (!g_log_cfg.fileFailureReported)
    {
        std::fprintf(stderr,
                     "MemoChatQml: file logging disabled because owner-only permissions could not be enforced\n");
        g_log_cfg.fileFailureReported = true;
    }
    if (g_log_cfg.toConsole)
    {
        std::fprintf(stderr, "%s\n", line.constData());
    }

    if (type == QtFatalMsg)
    {
        abort();
    }
}
