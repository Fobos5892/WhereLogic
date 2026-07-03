QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase c++17
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../../game/src

SOURCES += \
    tst_round_reveal_policy.cpp \
    ../../game/src/core/RoundRevealPolicy.cpp

HEADERS += \
    ../../game/src/core/GameConstants.h \
    ../../game/src/core/RoundRevealPolicy.h

TARGET = tst_round_reveal_policy
