QT += testlib network
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase c++17
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += \
    ../../game/src \
    ../../shared/api \
    ../../presenter/src

SOURCES += \
    tst_restapiclient.cpp \
    ../../presenter/src/RestApiClient.cpp

HEADERS += \
    ../../presenter/src/RestApiClient.h

TARGET = tst_restapiclient
