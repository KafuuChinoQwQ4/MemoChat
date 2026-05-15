#pragma once

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QString>
#include <QStringList>

class PetSpeechSynthesizer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool speaking READ speaking NOTIFY speakingChanged)

public:
    explicit PetSpeechSynthesizer(QObject *parent = nullptr);
    ~PetSpeechSynthesizer() override;

    bool speaking() const { return _speaking; }

    Q_INVOKABLE bool speak(const QString &text, const QString &locale = QString());
    Q_INVOKABLE void stop();

signals:
    void speakingChanged();
    void errorOccurred(const QString &message);

private:
    bool startSpeechProcess(const QString &program, const QStringList &arguments);
    void setSpeaking(bool speaking);

    QPointer<QProcess> _process;
    bool _speaking = false;
};
