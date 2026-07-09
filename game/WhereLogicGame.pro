QT += core gui quick sql network multimedia concurrent

include(../third_party/httpserver.pri)

CONFIG += c++17 utf8_source no_whisper

TARGET = WhereLogicGame
TEMPLATE = app

INCLUDEPATH += \
    ../shared/api \
    src

exists(../third_party/opencv.pri) {
    include(../third_party/opencv.pri)
}
exists(../third_party/opencv_fetch.pri) {
    include(../third_party/opencv_fetch.pri)
}

!contains(CONFIG, no_opencv) {
    DEFINES += HAS_OPENCV
}

SOURCES += \
    main.cpp \
    src/core/RoundRevealPolicy.cpp \
    src/core/UiStringDefaults.cpp \
    src/models/AudioSettings.cpp \
    src/models/DatabaseManager.cpp \
    src/models/ImageProcessor.cpp \
    src/models/PuzzleImageProvider.cpp \
    src/models/NetworkServer.cpp \
    src/viewmodels/AdminViewModel.cpp \
    src/viewmodels/GameViewModel.cpp

HEADERS += \
    src/core/GameConstants.h \
    src/core/RoundRevealPolicy.h \
    src/core/UiStringDefaults.h \
    src/models/AudioSettings.h \
    src/models/DatabaseManager.h \
    src/models/ImageProcessor.h \
    src/models/PuzzleImageProvider.h \
    src/models/NetworkServer.h \
    src/viewmodels/AdminViewModel.h \
    src/viewmodels/GameViewModel.h

RESOURCES += resources.qrc
