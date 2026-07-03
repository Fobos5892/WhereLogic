#include "models/AiRuntimeManager.h"
#include "models/AudioSettings.h"
#include "models/DatabaseManager.h"
#include "models/ImageProcessor.h"
#include "models/NetworkServer.h"
#include "models/VoiceRecognizer.h"
#include "models/PuzzleImageProvider.h"
#include "viewmodels/AdminViewModel.h"
#include "viewmodels/GameViewModel.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("WhereLogic"));
    app.setApplicationName(QStringLiteral("WhereLogicGame"));

    DatabaseManager database;
    if (!database.initialize()) {
        return -1;
    }

    GameViewModel gameViewModel(&database);
    AdminViewModel adminViewModel(&database);
    NetworkServer networkServer;
    ImageProcessor imageProcessor;
    VoiceRecognizer voiceRecognizer;
    AiRuntimeManager aiRuntime;
    AudioSettings audioSettings;
    PuzzleImageProvider puzzleImageProvider(&database);

    puzzleImageProvider.setImageProcessor(&imageProcessor);
    adminViewModel.setImageProcessor(&imageProcessor);
    adminViewModel.setImageProvider(&puzzleImageProvider);

    networkServer.setGameViewModel(&gameViewModel);
    gameViewModel.setNetworkServer(&networkServer);
    if (!networkServer.start()) {
        return -2;
    }

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("puzzle"), &puzzleImageProvider);
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    engine.addImportPath(QStringLiteral("qrc:/qml/components"));
    QQmlContext *context = engine.rootContext();
    context->setContextProperty(QStringLiteral("gameViewModel"), &gameViewModel);
    context->setContextProperty(QStringLiteral("adminViewModel"), &adminViewModel);
    context->setContextProperty(QStringLiteral("networkServer"), &networkServer);
    context->setContextProperty(QStringLiteral("imageProcessor"), &imageProcessor);
    context->setContextProperty(QStringLiteral("voiceRecognizer"), &voiceRecognizer);
    context->setContextProperty(QStringLiteral("aiRuntime"), &aiRuntime);
    context->setContextProperty(QStringLiteral("audioSettings"), &audioSettings);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return QGuiApplication::exec();
}
