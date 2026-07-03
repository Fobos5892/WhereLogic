#include "AiRuntimeManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTimer>

namespace {
constexpr QLatin1String kModelFileName("ggml-tiny.bin");
constexpr QLatin1String kModelRuFileName("ggml-tiny-ru.bin");
} // namespace

AiRuntimeManager::AiRuntimeManager(QObject *parent)
    : QObject(parent)
{
    resolvePaths();
}

void AiRuntimeManager::resolvePaths()
{
    const QString bundled = resolveBundledModelPath();
    const QString appData = resolveAppDataModelPath();

    if (QFile::exists(bundled)) {
        m_whisperModelPath = bundled;
    } else if (QFile::exists(appData)) {
        m_whisperModelPath = appData;
    } else {
        m_whisperModelPath = appData;
    }

    m_modelAvailable = QFile::exists(m_whisperModelPath);
    emit whisperModelPathChanged(m_whisperModelPath);
    emit modelAvailableChanged(m_modelAvailable);
}

QString AiRuntimeManager::resolveBundledModelPath() const
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString ruPath = appDir.filePath(QStringLiteral("models/%1").arg(QString(kModelRuFileName)));
    if (QFile::exists(ruPath)) {
        return ruPath;
    }
    return appDir.filePath(QStringLiteral("models/%1").arg(QString(kModelFileName)));
}

QString AiRuntimeManager::resolveAppDataModelPath() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base + QStringLiteral("/models"));
    const QString ruPath = base + QStringLiteral("/models/") + QString(kModelRuFileName);
    if (QFile::exists(ruPath)) {
        return ruPath;
    }
    return base + QStringLiteral("/models/") + QString(kModelFileName);
}

void AiRuntimeManager::downloadWhisperModel()
{
    if (m_downloadInProgress) {
        return;
    }

    m_downloadInProgress = true;
    emit downloadInProgressChanged(true);

    // Stub: real download will be wired to qmake extra targets / QNetworkAccessManager.
    QTimer::singleShot(500, this, [this]() {
        m_downloadInProgress = false;
        emit downloadInProgressChanged(false);
        emit downloadFinished(false,
                              QStringLiteral("Automatic model download is not configured in this build."));
    });
}
