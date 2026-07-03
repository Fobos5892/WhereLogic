#pragma once

#include <QObject>
#include <QString>

class TriggerParser;

class VoiceRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit VoiceRecognizer(QObject *parent = nullptr);

    Q_PROPERTY(bool available READ isAvailable CONSTANT)
    Q_PROPERTY(bool listening READ isListening NOTIFY listeningChanged)

    bool isAvailable() const;
    bool isListening() const { return m_listening; }

    Q_INVOKABLE void startListening();
    Q_INVOKABLE void stopListening();
    Q_INVOKABLE QString parseTriggerFromText(const QString &transcript) const;

signals:
    void listeningChanged(bool listening);
    void answerDetected(const QString &answer);
    void recognitionFailed(const QString &reason);

private:
    void setListening(bool listening);

    bool m_listening = false;
};
