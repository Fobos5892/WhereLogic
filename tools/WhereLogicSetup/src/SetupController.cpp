#include "SetupController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStandardPaths>

namespace {

QString findRepoRoot()
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 10; ++i) {
        if (QFile::exists(dir.filePath(QStringLiteral("WhereLogic.pro")))
            || QFile::exists(dir.filePath(QStringLiteral("config/deps_manifest.json"))))
            return dir.absolutePath();
        if (!dir.cdUp())
            break;
    }
    return QDir::currentPath();
}

void copyTree(const QString &from, const QString &to)
{
    QDir().mkpath(to);
    QDir src(from);
    const QFileInfoList entries = src.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : entries) {
        const QString dst = QDir(to).filePath(fi.fileName());
        if (fi.isDir())
            copyTree(fi.absoluteFilePath(), dst);
        else
            QFile::copy(fi.absoluteFilePath(), dst);
    }
}

} // namespace

SetupController::SetupController(QObject *parent)
    : QObject(parent)
    , m_repoRoot(findRepoRoot())
{
    connect(&m_process, &QProcess::finished, this, &SetupController::onProcessFinished);
    loadManifest();
}

void SetupController::appendLog(const QString &line)
{
    m_log.append(line);
    emit logTextChanged();
}

void SetupController::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void SetupController::setTotalProgress(qreal value)
{
    value = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(m_totalProgress, value))
        return;
    m_totalProgress = value;
    emit totalProgressChanged();
}

bool SetupController::loadManifest()
{
    const QString path = QDir(m_repoRoot).filePath(QStringLiteral("config/deps_manifest.json"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        appendLog(QStringLiteral("Не найден манифест: %1").arg(path));
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonArray components = doc.object().value(QStringLiteral("components")).toArray();
    m_model.loadFromManifest(components, m_repoRoot);
    appendLog(QStringLiteral("Репозиторий: %1").arg(m_repoRoot));
    return true;
}

void SetupController::refresh()
{
    m_model.refreshInstalled(m_repoRoot);
    appendLog(QStringLiteral("Статус обновлён."));
}

bool SetupController::platformOk(const QJsonObject &item) const
{
    const QJsonArray platforms = item.value(QStringLiteral("platforms")).toArray();
    if (platforms.isEmpty())
        return true;
#if defined(Q_OS_WIN)
    return platforms.contains(QStringLiteral("win32"));
#elif defined(Q_OS_MACOS)
    return platforms.contains(QStringLiteral("darwin"));
#else
    return platforms.contains(QStringLiteral("linux"));
#endif
}

QString SetupController::absPath(const QString &rel) const
{
    return QDir(m_repoRoot).filePath(rel);
}

void SetupController::toggleRow(int row, bool selected)
{
    const QModelIndex idx = m_model.index(row);
    m_model.setData(idx, selected, DepListModel::SelectedRole);
}

void SetupController::fetchRow(int row)
{
    enqueueRows({row});
}

void SetupController::fetchSelected()
{
    enqueueRows(m_model.selectedRows());
}

void SetupController::fetchAll()
{
    QList<int> rows;
    for (int i = 0; i < m_model.rowCount(); ++i) {
        const QJsonObject item = m_model.itemAt(i);
        if (item.value(QStringLiteral("kind")).toString() == QStringLiteral("manual"))
            continue;
        rows.append(i);
    }
    enqueueRows(rows);
}

void SetupController::enqueueRows(const QList<int> &rows)
{
    if (m_busy) {
        appendLog(QStringLiteral("Уже выполняется загрузка…"));
        return;
    }

    m_queue.clear();
    for (int row : rows) {
        const QJsonObject item = m_model.itemAt(row);
        const QString kind = item.value(QStringLiteral("kind")).toString();
        if (kind == QStringLiteral("manual")) {
            m_model.setStatus(row,
                              QStringLiteral("manual"),
                              item.value(QStringLiteral("manual_hint")).toString(),
                              0.0);
            continue;
        }
        if (!platformOk(item)) {
            m_model.setStatus(row,
                              QStringLiteral("skipped"),
                              QStringLiteral("Не для этой платформы"),
                              0.0);
            continue;
        }
        const QString verify = item.value(QStringLiteral("verify_path")).toString();
        if (!verify.isEmpty() && QFile::exists(absPath(verify))) {
            m_model.setStatus(row, QStringLiteral("installed"), QStringLiteral("Уже установлено"), 1.0);
            continue;
        }
        m_queue.enqueue(row);
    }

    if (m_queue.isEmpty()) {
        appendLog(QStringLiteral("Нечего загружать."));
        return;
    }

    m_queueDone = 0;
    m_queueTotal = m_queue.size();
    setTotalProgress(0.0);
    setBusy(true);
    appendLog(QStringLiteral("Старт: %1 компонент(ов)").arg(m_queueTotal));
    startNext();
}

void SetupController::startNext()
{
    if (m_queue.isEmpty()) {
        setBusy(false);
        setTotalProgress(1.0);
        appendLog(QStringLiteral("Готово."));
        refresh();
        return;
    }

    m_activeRow = m_queue.dequeue();
    const QJsonObject item = m_model.itemAt(m_activeRow);
    const QString kind = item.value(QStringLiteral("kind")).toString();
    const QString label = item.value(QStringLiteral("label")).toString();

    appendLog(QStringLiteral("→ %1").arg(label));
    m_model.setStatus(m_activeRow, QStringLiteral("downloading"), QStringLiteral("Загрузка…"), 0.0);

    if (kind == QStringLiteral("git")) {
        installGit(m_activeRow, item);
    } else if (kind == QStringLiteral("http")) {
        installHttp(m_activeRow, item);
    } else if (kind == QStringLiteral("opencv_windows")) {
        installOpenCv(m_activeRow, item);
    } else {
        finishCurrent(false, QStringLiteral("Неизвестный тип: %1").arg(kind));
    }
}

void SetupController::installGit(int row, const QJsonObject &item)
{
    const QString dest = absPath(item.value(QStringLiteral("destination")).toString());
    QDir(dest).removeRecursively();

    m_model.setStatus(row, QStringLiteral("downloading"), QStringLiteral("git clone…"), 0.1);
    m_process.setWorkingDirectory(m_repoRoot);
    m_process.start(QStringLiteral("git"),
                    {QStringLiteral("clone"),
                     QStringLiteral("--depth"),
                     QStringLiteral("1"),
                     item.value(QStringLiteral("url")).toString(),
                     dest});
}

void SetupController::installHttp(int row, const QJsonObject &item)
{
    const QString dest = absPath(item.value(QStringLiteral("destination")).toString());
    QDir().mkpath(QFileInfo(dest).absolutePath());

    const QString part = dest + QStringLiteral(".part");
    if (QFile::exists(part))
        QFile::remove(part);

    m_outFile = new QFile(part, this);
    if (!m_outFile->open(QIODevice::WriteOnly)) {
        finishCurrent(false, QStringLiteral("Не удалось создать файл"));
        return;
    }

    QNetworkRequest req(QUrl(item.value(QStringLiteral("url")).toString()));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("WhereLogic-Setup/1.0"));
    m_reply = m_network.get(req);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &SetupController::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &SetupController::onDownloadFinished);
    Q_UNUSED(row)
}

void SetupController::installOpenCv(int row, const QJsonObject &item)
{
    m_pendingOpenCvDest = absPath(item.value(QStringLiteral("destination")).toString());
    m_pendingOpenCvVersion = item.value(QStringLiteral("version")).toString(QStringLiteral("4.10.0"));

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString installer = QDir(tempDir).filePath(
        QStringLiteral("opencv-%1-windows.exe").arg(m_pendingOpenCvVersion));

    if (QFile::exists(installer)) {
        m_model.setStatus(row, QStringLiteral("extracting"), QStringLiteral("Распаковка OpenCV…"), 0.5);
        const QString extractRoot = QDir(tempDir).filePath(
            QStringLiteral("opencv-extract-%1").arg(m_pendingOpenCvVersion));
        QDir().mkpath(extractRoot);
        m_process.start(installer, {QStringLiteral("-o%1").arg(extractRoot), QStringLiteral("-y")});
        return;
    }

    const QString part = installer + QStringLiteral(".part");
    m_outFile = new QFile(part, this);
    if (!m_outFile->open(QIODevice::WriteOnly)) {
        finishCurrent(false, QStringLiteral("Не удалось создать временный файл"));
        return;
    }

    QNetworkRequest req(QUrl(item.value(QStringLiteral("url")).toString()));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("WhereLogic-Setup/1.0"));
    m_reply = m_network.get(req);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &SetupController::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, [this, installer, row]() {
        if (!m_reply || m_reply->error() != QNetworkReply::NoError) {
            finishCurrent(false, m_reply ? m_reply->errorString() : QStringLiteral("HTTP error"));
            return;
        }
        m_outFile->close();
        QFile::rename(m_outFile->fileName(), installer);
        m_outFile->deleteLater();
        m_outFile = nullptr;
        m_reply->deleteLater();
        m_reply = nullptr;

        m_model.setStatus(row, QStringLiteral("extracting"), QStringLiteral("Распаковка OpenCV…"), 0.5);
        const QString extractRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                        .filePath(QStringLiteral("opencv-extract-%1").arg(m_pendingOpenCvVersion));
        QDir().mkpath(extractRoot);
        m_process.start(installer, {QStringLiteral("-o%1").arg(extractRoot), QStringLiteral("-y")});
    });
}

void SetupController::onDownloadProgress(qint64 received, qint64 total)
{
    qreal p = total > 0 ? qreal(received) / qreal(total) : 0.0;
    if (m_activeRow >= 0)
        m_model.setStatus(m_activeRow, QStringLiteral("downloading"), QStringLiteral("Загрузка…"), p);

    const qreal queueBase = qreal(m_queueDone) / qreal(qMax(1, m_queueTotal));
    const qreal queueSlice = 1.0 / qreal(qMax(1, m_queueTotal));
    setTotalProgress(queueBase + p * queueSlice * 0.9);
}

void SetupController::onDownloadFinished()
{
    if (!m_reply)
        return;

    if (m_reply->error() != QNetworkReply::NoError) {
        finishCurrent(false, m_reply->errorString());
        return;
    }

    const QJsonObject item = m_model.itemAt(m_activeRow);
    const QString dest = absPath(item.value(QStringLiteral("destination")).toString());
    const QString part = m_outFile->fileName();
    m_outFile->close();
    if (QFile::exists(dest))
        QFile::remove(dest);
    QFile::rename(part, dest);

    m_reply->deleteLater();
    m_reply = nullptr;
    m_outFile->deleteLater();
    m_outFile = nullptr;

    finishCurrent(true, QStringLiteral("Сохранено"));
}

void SetupController::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit || exitCode != 0) {
        finishCurrent(false, QStringLiteral("Процесс завершился с кодом %1").arg(exitCode));
        return;
    }

    if (!m_pendingOpenCvDest.isEmpty()) {
        const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        const QString extractRoot = QDir(tempDir).filePath(
            QStringLiteral("opencv-extract-%1").arg(m_pendingOpenCvVersion));
        const QString srcRoot = QDir(extractRoot).filePath(QStringLiteral("opencv/build"));
        const QString srcInclude = QDir(srcRoot).filePath(QStringLiteral("include"));
        const QString srcLib = QDir(srcRoot).filePath(QStringLiteral("x64/vc16/lib"));
        const QString srcBin = QDir(srcRoot).filePath(QStringLiteral("x64/vc16/bin"));

        QDir(m_pendingOpenCvDest).removeRecursively();
        QDir().mkpath(m_pendingOpenCvDest + QStringLiteral("/include"));
        const QString msvcLib = m_pendingOpenCvDest + QStringLiteral("/x64/msvc/lib");
        const QString msvcBin = m_pendingOpenCvDest + QStringLiteral("/x64/msvc/bin");
        QDir().mkpath(msvcLib);
        QDir().mkpath(msvcBin);
        copyTree(srcInclude, m_pendingOpenCvDest + QStringLiteral("/include"));
        for (const QFileInfo &fi : QDir(srcLib).entryInfoList({QStringLiteral("*.lib")}, QDir::Files))
            QFile::copy(fi.absoluteFilePath(), msvcLib + QStringLiteral("/") + fi.fileName());
        for (const QFileInfo &fi :
             QDir(srcBin).entryInfoList({QStringLiteral("opencv_world*.dll")}, QDir::Files))
            QFile::copy(fi.absoluteFilePath(), msvcBin + QStringLiteral("/") + fi.fileName());

        for (const QString &legacy : {QStringLiteral("/lib"), QStringLiteral("/bin")}) {
            QDir dir(m_pendingOpenCvDest + legacy);
            if (dir.exists())
                dir.removeRecursively();
        }

        m_pendingOpenCvDest.clear();
        finishCurrent(true, QStringLiteral("OpenCV MSVC → x64/msvc (MSVC kit only)"));
        return;
    }

    finishCurrent(true, QStringLiteral("Готово"));
}

void SetupController::finishCurrent(bool ok, const QString &message)
{
    if (m_activeRow >= 0) {
        m_model.setStatus(m_activeRow,
                          ok ? QStringLiteral("installed") : QStringLiteral("error"),
                          message,
                          ok ? 1.0 : 0.0);
    }
    appendLog(ok ? QStringLiteral("  ✓ %1").arg(message)
                 : QStringLiteral("  ✗ %1").arg(message));

    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_outFile) {
        m_outFile->close();
        m_outFile->deleteLater();
        m_outFile = nullptr;
    }

    ++m_queueDone;
    setTotalProgress(qreal(m_queueDone) / qreal(qMax(1, m_queueTotal)));
    m_activeRow = -1;
    startNext();
}
