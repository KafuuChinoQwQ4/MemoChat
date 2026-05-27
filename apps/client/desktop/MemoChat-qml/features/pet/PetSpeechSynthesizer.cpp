#include "PetSpeechSynthesizer.h"

#include <QByteArray>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QSysInfo>
#include <QtGlobal>

namespace {

bool commandExists(const QString &program)
{
    if (program.contains(QLatin1Char('/')) || program.contains(QLatin1Char('\\'))) {
        return QFile::exists(program);
    }
    return !QStandardPaths::findExecutable(program).isEmpty();
}

bool isWslRuntime()
{
    return !qEnvironmentVariableIsEmpty("WSL_INTEROP")
           || !qEnvironmentVariableIsEmpty("WSL_DISTRO_NAME");
}

QByteArray utf16LeBase64(const QString &text)
{
    QByteArray bytes;
    bytes.reserve(text.size() * 2);
    for (const QChar ch : text) {
        const ushort value = ch.unicode();
        bytes.append(static_cast<char>(value & 0xff));
        bytes.append(static_cast<char>((value >> 8) & 0xff));
    }
    return bytes.toBase64();
}

QString powerShellExecutable()
{
#ifdef Q_OS_WIN
    return QStringLiteral("powershell.exe");
#else
    if (commandExists(QStringLiteral("powershell.exe"))) {
        return QStringLiteral("powershell.exe");
    }
    const QString windowsPowerShell =
        QStringLiteral("/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe");
    return QFile::exists(windowsPowerShell) ? windowsPowerShell : QString();
#endif
}

QStringList powerShellSpeakArguments(const QString &text, const QString &locale)
{
    const QString textBase64 = QString::fromLatin1(text.toUtf8().toBase64());
    const QString localeBase64 = QString::fromLatin1(locale.toUtf8().toBase64());
    const QString command = QStringLiteral(
        "$ErrorActionPreference='Stop';"
        "$bytes=[Convert]::FromBase64String('%1');"
        "$text=[Text.Encoding]::UTF8.GetString($bytes);"
        "$localeBytes=[Convert]::FromBase64String('%2');"
        "$locale=[Text.Encoding]::UTF8.GetString($localeBytes).ToLowerInvariant();"
        "$culture='zh*';"
        "if($locale.StartsWith('ja') -or $locale -eq 'jp'){$culture='ja*'} "
        "elseif($locale.StartsWith('en')){$culture='en*'} "
        "elseif($locale.StartsWith('ko')){$culture='ko*'};"
        "Add-Type -AssemblyName System.Speech;"
        "$speaker=New-Object System.Speech.Synthesis.SpeechSynthesizer;"
        "$speaker.Volume=100;"
        "$speaker.Rate=0;"
        "try{"
        "$voice=$speaker.GetInstalledVoices()|Where-Object{$_.VoiceInfo.Culture.Name -like $culture}|Select-Object -First 1;"
        "if($voice){$speaker.SelectVoice($voice.VoiceInfo.Name)}"
        "}catch{};"
        "$speaker.Speak($text);").arg(textBase64, localeBase64);

    return {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-NonInteractive"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-WindowStyle"),
        QStringLiteral("Hidden"),
        QStringLiteral("-EncodedCommand"),
        QString::fromLatin1(utf16LeBase64(command)),
    };
}

QString espeakVoiceForLocale(const QString &locale)
{
    const QString normalized = locale.trimmed().toLower();
    if (normalized.startsWith(QStringLiteral("zh"))) {
        return QStringLiteral("zh");
    }
    if (normalized.startsWith(QStringLiteral("ja"))) {
        return QStringLiteral("ja");
    }
    if (normalized.startsWith(QStringLiteral("ko"))) {
        return QStringLiteral("ko");
    }
    return QStringLiteral("en");
}

} // namespace

PetSpeechSynthesizer::PetSpeechSynthesizer(QObject *parent)
    : QObject(parent)
{
}

PetSpeechSynthesizer::~PetSpeechSynthesizer()
{
    stop();
}

bool PetSpeechSynthesizer::speak(const QString &text, const QString &locale)
{
    const QString spokenText = text.trimmed();
    const QString requestedLocale = locale.trimmed().isEmpty() ? QStringLiteral("zh-CN") : locale.trimmed();
    if (spokenText.isEmpty()) {
        stop();
        return false;
    }

    stop();

    const QString ps = powerShellExecutable();
#if defined(Q_OS_WIN)
    if (!ps.isEmpty() && startSpeechProcess(ps, powerShellSpeakArguments(spokenText, requestedLocale))) {
        return true;
    }
#else
    if (isWslRuntime() && !ps.isEmpty()
        && startSpeechProcess(ps, powerShellSpeakArguments(spokenText, requestedLocale))) {
        return true;
    }
    if (commandExists(QStringLiteral("spd-say"))
        && startSpeechProcess(QStringLiteral("spd-say"),
                              {QStringLiteral("-w"), QStringLiteral("-l"), requestedLocale, spokenText})) {
        return true;
    }
    if (commandExists(QStringLiteral("espeak-ng"))
        && startSpeechProcess(QStringLiteral("espeak-ng"),
                              {QStringLiteral("-v"), espeakVoiceForLocale(requestedLocale), spokenText})) {
        return true;
    }
    if (commandExists(QStringLiteral("espeak"))
        && startSpeechProcess(QStringLiteral("espeak"),
                              {QStringLiteral("-v"), espeakVoiceForLocale(requestedLocale), spokenText})) {
        return true;
    }
    if (!ps.isEmpty() && startSpeechProcess(ps, powerShellSpeakArguments(spokenText, requestedLocale))) {
        return true;
    }
#endif

    emit errorOccurred(QStringLiteral("No desktop text-to-speech backend is available"));
    return false;
}

void PetSpeechSynthesizer::stop()
{
    if (_process) {
        _process->disconnect(this);
        if (_process->state() != QProcess::NotRunning) {
            _process->kill();
            _process->waitForFinished(250);
        }
        _process->deleteLater();
        _process.clear();
    }
    setSpeaking(false);
}

bool PetSpeechSynthesizer::startSpeechProcess(const QString &program, const QStringList &arguments)
{
    auto *process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(arguments);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, process](int, QProcess::ExitStatus) {
        if (_process == process) {
            _process.clear();
            setSpeaking(false);
        }
        process->deleteLater();
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError) {
        const QString message = process->errorString();
        if (_process == process) {
            _process.clear();
            setSpeaking(false);
        }
        emit errorOccurred(message);
        process->deleteLater();
    });

    process->start();
    if (!process->waitForStarted(800)) {
        const QString message = process->errorString();
        process->deleteLater();
        emit errorOccurred(message);
        return false;
    }

    _process = process;
    setSpeaking(true);
    return true;
}

void PetSpeechSynthesizer::setSpeaking(bool speaking)
{
    if (_speaking == speaking) {
        return;
    }
    _speaking = speaking;
    emit speakingChanged();
}
