QT += testlib sql
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase c++17
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += \
    .. \
    ../../game/src \
    ../../shared/api

SOURCES += \
    tst_databasemanager.cpp \
    ../../game/src/core/UiStringDefaults.cpp \
    ../../game/src/models/DatabaseManager.cpp

HEADERS += \
    ../../game/src/core/UiStringDefaults.h \
    ../../game/src/models/DatabaseManager.h \
    ../../game/src/core/GameConstants.h

TARGET = tst_databasemanager
