QT += testlib sql network
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase c++17
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += \
    ../../game/src \
    ../../shared/api

SOURCES += \
    tst_networkserver.cpp \
    ../../game/src/core/RoundRevealPolicy.cpp \
    ../../game/src/core/UiStringDefaults.cpp \
    ../../game/src/models/DatabaseManager.cpp \
    ../../game/src/models/NetworkServer.cpp \
    ../../game/src/models/TriggerParser.cpp \
    ../../game/src/viewmodels/GameViewModel.cpp

HEADERS += \
    ../../game/src/core/GameConstants.h \
    ../../game/src/core/RoundRevealPolicy.h \
    ../../game/src/core/UiStringDefaults.h \
    ../../game/src/models/DatabaseManager.h \
    ../../game/src/models/NetworkServer.h \
    ../../game/src/models/TriggerParser.h \
    ../../game/src/viewmodels/GameViewModel.h

include(../../third_party/httpserver.pri)

TARGET = tst_networkserver
