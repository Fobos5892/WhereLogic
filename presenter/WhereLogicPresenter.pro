QT += core gui quick network

CONFIG += c++17

TARGET = WhereLogicPresenter
TEMPLATE = app

INCLUDEPATH += \
    ../shared/api \
    src

SOURCES += \
    main.cpp \
    src/RestApiClient.cpp \
    src/PresenterViewModel.cpp

HEADERS += \
    src/RestApiClient.h \
    src/PresenterViewModel.h

RESOURCES += resources.qrc
