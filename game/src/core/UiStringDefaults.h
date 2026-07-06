#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class UiStringDefaults
{
public:
    static UiStringDefaults &instance();

    bool loadFromResource(const QString &resourcePath = QStringLiteral(":/config/ui_defaults_ru.json"));
    QString text(const QString &key) const;
    bool contains(const QString &key) const;
    QStringList keys() const;

private:
    UiStringDefaults() = default;

    QHash<QString, QString> m_strings;
};
