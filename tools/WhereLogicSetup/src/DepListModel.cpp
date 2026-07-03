#include "DepListModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

DepListModel::DepListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int DepListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant DepListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};

    const Row &row = m_rows.at(index.row());
    switch (role) {
    case IdRole:
        return row.item.value(QStringLiteral("id")).toString();
    case LabelRole:
        return row.item.value(QStringLiteral("label")).toString();
    case KindRole:
        return row.item.value(QStringLiteral("kind")).toString();
    case StatusRole:
        return row.status;
    case StatusTextRole:
        return row.statusText;
    case ProgressRole:
        return row.progress;
    case InstalledRole:
        return row.installed;
    case ManualHintRole:
        return row.item.value(QStringLiteral("manual_hint")).toString();
    case SelectedRole:
        return row.selected;
    default:
        return {};
    }
}

bool DepListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return false;

    if (role == SelectedRole) {
        m_rows[index.row()].selected = value.toBool();
        emit dataChanged(index, index, {SelectedRole});
        return true;
    }
    return false;
}

QHash<int, QByteArray> DepListModel::roleNames() const
{
    return {
        {IdRole, "depId"},
        {LabelRole, "label"},
        {KindRole, "kind"},
        {StatusRole, "status"},
        {StatusTextRole, "statusText"},
        {ProgressRole, "progress"},
        {InstalledRole, "installed"},
        {ManualHintRole, "manualHint"},
        {SelectedRole, "selected"},
    };
}

bool DepListModel::verifyInstalled(const QJsonObject &item, const QString &repoRoot)
{
    const QString kind = item.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("manual"))
        return false;

    const QString verify = item.value(QStringLiteral("verify_path")).toString();
    if (verify.isEmpty())
        return false;

    return QFile::exists(QDir(repoRoot).filePath(verify));
}

void DepListModel::loadFromManifest(const QJsonArray &components, const QString &repoRoot)
{
    beginResetModel();
    m_rows.clear();
    for (const QJsonValue &v : components) {
        const QJsonObject obj = v.toObject();
        Row row;
        row.item = obj;
        row.installed = verifyInstalled(obj, repoRoot);
        row.status = row.installed ? QStringLiteral("installed") : QStringLiteral("pending");
        row.statusText = row.installed ? QStringLiteral("Уже установлено")
                                       : (obj.value(QStringLiteral("kind")).toString()
                                              == QStringLiteral("manual")
                                            ? QStringLiteral("Вручную через Qt Maintenance Tool")
                                            : QStringLiteral("Ожидает загрузки"));
        row.selected = obj.value(QStringLiteral("kind")).toString()
                       != QStringLiteral("manual");
        m_rows.append(row);
    }
    endResetModel();
}

int DepListModel::indexById(const QString &id) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).item.value(QStringLiteral("id")).toString() == id)
            return i;
    }
    return -1;
}

QString DepListModel::idAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return {};
    return m_rows.at(row).item.value(QStringLiteral("id")).toString();
}

QJsonObject DepListModel::itemAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return {};
    return m_rows.at(row).item;
}

void DepListModel::setStatus(int row, const QString &status, const QString &text, qreal progress)
{
    if (row < 0 || row >= m_rows.size())
        return;

    m_rows[row].status = status;
    m_rows[row].statusText = text;
    m_rows[row].progress = progress;
    if (status == QStringLiteral("installed"))
        m_rows[row].installed = true;

    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {StatusRole, StatusTextRole, ProgressRole, InstalledRole});
}

void DepListModel::refreshInstalled(const QString &repoRoot)
{
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i].installed = verifyInstalled(m_rows[i].item, repoRoot);
        if (m_rows[i].status != QStringLiteral("downloading")
            && m_rows[i].status != QStringLiteral("extracting")) {
            m_rows[i].status = m_rows[i].installed ? QStringLiteral("installed")
                                                   : QStringLiteral("pending");
            m_rows[i].statusText = m_rows[i].installed ? QStringLiteral("Уже установлено")
                                                       : QStringLiteral("Ожидает загрузки");
        }
        const QModelIndex idx = index(i);
        emit dataChanged(idx, idx, {StatusRole, StatusTextRole, InstalledRole});
    }
}

QList<int> DepListModel::selectedRows() const
{
    QList<int> rows;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).selected)
            rows.append(i);
    }
    return rows;
}
