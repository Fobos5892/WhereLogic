#include "UiStringDefaults.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

UiStringDefaults &UiStringDefaults::instance()
{
    static UiStringDefaults defaults;
    return defaults;
}

bool UiStringDefaults::loadFromResource(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonObject strings = root.value(QStringLiteral("strings")).toObject();
    m_strings.clear();
    for (auto it = strings.begin(); it != strings.end(); ++it) {
        m_strings.insert(it.key(), it.value().toString());
    }
    return !m_strings.isEmpty();
}

QString UiStringDefaults::text(const QString &key) const
{
    return m_strings.value(key);
}

bool UiStringDefaults::contains(const QString &key) const
{
    return m_strings.contains(key);
}
