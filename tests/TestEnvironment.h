#pragma once

#include <QCoreApplication>
#include <QStandardPaths>
#include <QtTest>

inline void setupTestAppPathsBeforeApp()
{
    QCoreApplication::setOrganizationName(QStringLiteral("WhereLogic"));
    QCoreApplication::setApplicationName(QStringLiteral("WhereLogicTests"));
}

inline void setupTestAppPathsAfterApp()
{
    QStandardPaths::setTestModeEnabled(true);
}

#define WHERLOGIC_QTEST_MAIN(TestObject) \
    int main(int argc, char *argv[]) \
    { \
        setupTestAppPathsBeforeApp(); \
        QCoreApplication app(argc, argv); \
        setupTestAppPathsAfterApp(); \
        TestObject testObject; \
        return QTest::qExec(&testObject, argc, argv); \
    }
