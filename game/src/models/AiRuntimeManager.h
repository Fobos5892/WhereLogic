#pragma once

#include <QObject>
#include <QString>

class AiRuntimeManager : public QObject
{
    Q_OBJECT

public:
    explicit AiRuntimeManager(QObject *parent = nullptr);

    Q_PROPERTY(QString whisperModelPath READ whisperModelPath NOTIFY whisperModelPathChanged)
    Q_PROPERTY(bool modelAvailable READ isModelAvailable NOTIFY modelAvailableChanged)
    Q_PROPERTY(bool downloadInProgress READ isDownloadInProgress NOTIFY downloadInProgressChanged)

    QString whisperModelPath() const { return m_whisperModelPath; }
    bool isModelAvailable() const { return m_modelAvailable; }
    bool isDownloadInProgress() const { return m_downloadInProgress; }

    Q_INVOKABLE void resolvePaths();
    Q_INVOKABLE void downloadWhisperModel();

signals:
    void whisperModelPathChanged(const QString &path);
    void modelAvailableChanged(bool available);
    void downloadInProgressChanged(bool inProgress);
    void downloadFinished(bool success, const QString &message);

private:
    QString resolveBundledModelPath() const;
    QString resolveAppDataModelPath() const;

    QString m_whisperModelPath;
    bool m_modelAvailable = false;
    bool m_downloadInProgress = false;
};
