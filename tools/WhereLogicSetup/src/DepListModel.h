#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class DepListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        LabelRole,
        KindRole,
        StatusRole,
        StatusTextRole,
        ProgressRole,
        InstalledRole,
        ManualHintRole,
        SelectedRole,
    };
    Q_ENUM(Roles)

    explicit DepListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void loadFromManifest(const QJsonArray &components, const QString &repoRoot);
    int indexById(const QString &id) const;
    QString idAt(int row) const;
    QJsonObject itemAt(int row) const;
    void setStatus(int row, const QString &status, const QString &text, qreal progress);
    void refreshInstalled(const QString &repoRoot);
    QList<int> selectedRows() const;

private:
    struct Row {
        QJsonObject item;
        QString status;
        QString statusText;
        qreal progress = 0.0;
        bool installed = false;
        bool selected = true;
    };

    static bool verifyInstalled(const QJsonObject &item, const QString &repoRoot);

    QVector<Row> m_rows;
};
