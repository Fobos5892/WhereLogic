QT += core gui quick network

CONFIG += c++17

TARGET = WhereLogicSetup
TEMPLATE = app

INCLUDEPATH += src

SOURCES += \
    main.cpp \
    src/DepListModel.cpp \
    src/SetupController.cpp

HEADERS += \
    src/DepListModel.h \
    src/SetupController.h

RESOURCES += resources.qrc
