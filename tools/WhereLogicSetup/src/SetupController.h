#pragma once

#include "DepListModel.h"

#include <QFile>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QStringList>

class QNetworkReply;

class SetupController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(DepListModel *items READ items CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(qreal totalProgress READ totalProgress NOTIFY totalProgressChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QString repoRoot READ repoRoot CONSTANT)

public:
    explicit SetupController(QObject *parent = nullptr);

    DepListModel *items() { return &m_model; }
    bool busy() const { return m_busy; }
    qreal totalProgress() const { return m_totalProgress; }
    QString logText() const { return m_log.join(QStringLiteral("\n")); }
    QString repoRoot() const { return m_repoRoot; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void fetchSelected();
    Q_INVOKABLE void fetchAll();
    Q_INVOKABLE void fetchRow(int row);
    Q_INVOKABLE void toggleRow(int row, bool selected);

signals:
    void busyChanged();
    void totalProgressChanged();
    void logTextChanged();

private:
    void appendLog(const QString &line);
    void setBusy(bool busy);
    void setTotalProgress(qreal value);
    bool loadManifest();
    bool platformOk(const QJsonObject &item) const;
    void enqueueRows(const QList<int> &rows);
    void startNext();
    void finishCurrent(bool ok, const QString &message);
    void installGit(int row, const QJsonObject &item);
    void installHttp(int row, const QJsonObject &item);
    void installOpenCv(int row, const QJsonObject &item);
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    QString absPath(const QString &rel) const;

    DepListModel m_model;
    QNetworkAccessManager m_network;
    QProcess m_process;
    QNetworkReply *m_reply = nullptr;
    QFile *m_outFile = nullptr;
    QString m_repoRoot;
    QStringList m_log;
    QQueue<int> m_queue;
    int m_activeRow = -1;
    bool m_busy = false;
    qreal m_totalProgress = 0.0;
    int m_queueDone = 0;
    int m_queueTotal = 0;
    QString m_pendingOpenCvDest;
    QString m_pendingOpenCvVersion;
};
