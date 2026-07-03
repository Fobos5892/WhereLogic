QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase c++17 utf8_source
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../../game/src

SOURCES += \
    tst_triggerparser.cpp \
    ../../game/src/models/TriggerParser.cpp

HEADERS += ../../game/src/models/TriggerParser.h

TARGET = tst_triggerparser
