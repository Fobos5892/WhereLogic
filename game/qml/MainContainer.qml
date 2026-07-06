import QtQuick
import "."
import "components"

Item {
    id: root
    readonly property bool overlaysOpen: settingsOverlay.visible || adminEditorOverlay.visible

    readonly property var stages: ({
        "STAGE_WELCOME": "qrc:/qml/views/WelcomeScreen.qml",
        "STAGE_TEAM_SETUP": "qrc:/qml/views/TeamSetupScreen.qml",
        "STAGE_INTER_ROUND": "qrc:/qml/views/InterRoundScreen.qml",
        "STAGE_FINAL_VICTORY": "qrc:/qml/views/FinalVictoryScreen.qml"
    })

    function screenSource(stage) {
        if (stages[stage])
            return stages[stage]
        return "qrc:/qml/views/UniversalGameField.qml"
    }

    Loader {
        id: frontBuffer
        anchors.fill: parent
        source: screenSource(gameViewModel.currentStage)
    }

    MissingRevealOverlay {
        anchors.fill: parent
        visible: gameViewModel.currentStage === "STAGE_MISSING_REVEAL"
    }

    ScoreHUD {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.touchMin + Theme.spacing
        visible: gameViewModel.currentStage !== "STAGE_WELCOME"
                 && gameViewModel.currentStage !== "STAGE_TEAM_SETUP"
                 && !root.overlaysOpen
    }

    IconButton {
        id: settingsButton
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Theme.spacing
        visible: !root.overlaysOpen
        iconSource: "qrc:/qml/assets/icon-settings.svg"
        outline: true
        onClicked: settingsOverlay.visible = true
    }

    GameButton {
        id: resumeButton
        anchors.verticalCenter: settingsButton.verticalCenter
        anchors.right: settingsButton.left
        anchors.rightMargin: Theme.spacing * 0.75
        visible: !root.overlaysOpen
                 && gameViewModel.currentStage !== "STAGE_WELCOME"
                 && gameViewModel.currentStage !== "STAGE_TEAM_SETUP"
                 && gameViewModel.currentStage !== "STAGE_FINAL_VICTORY"
                 && gameViewModel.hasActiveSession
        text: gameViewModel.label("ui.welcome.continue")
        primary: false
        outline: true
        onClicked: gameViewModel.resumeSession()
    }

    Rectangle {
        id: settingsOverlay
        anchors.fill: parent
        color: "#CC0B0C10"
        visible: false
        z: 300

        MouseArea {
            anchors.fill: parent
            onClicked: settingsOverlay.visible = false
        }

        NeonPanel {
            id: settingsPanel
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.9, 480)

            Column {
                width: settingsPanel.innerWidth
                spacing: Theme.spacing

                Text {
                    width: parent.width
                    text: gameViewModel.label("ui.settings.title")
                    color: Theme.primary
                    font.pixelSize: Theme.fontSizeTitle
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    width: parent.width
                    text: gameViewModel.label("ui.settings.pin_hint")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    width: parent.width
                    text: networkServer.currentPin
                    color: Theme.gold
                    font.pixelSize: Theme.fontSizeHero
                    font.bold: true
                    font.letterSpacing: 10
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    width: parent.width
                    text: networkServer.remoteConnected
                          ? gameViewModel.label("ui.settings.remote_connected")
                          : gameViewModel.label("ui.settings.remote_waiting")
                    color: networkServer.remoteConnected ? Theme.success : Theme.textSecondary
                    font.pixelSize: Theme.fontSizeBody
                    horizontalAlignment: Text.AlignHCenter
                }

                GameButton {
                    width: parent.width
                    fillWidth: true
                    text: gameViewModel.label("ui.settings.content_editor")
                    gold: true
                    onClicked: {
                        settingsOverlay.visible = false
                        adminEditorOverlay.visible = true
                        adminViewModel.refreshPresets()
                        adminViewModel.refreshCatalogRounds()
                    }
                }

                Text {
                    width: parent.width
                    text: gameViewModel.label("ui.settings.volume")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
                }

                GameSlider {
                    width: parent.width
                    value: audioSettings.volume
                    onValueChanged: audioSettings.volume = value
                }

                GameButton {
                    width: parent.width
                    fillWidth: true
                    text: gameViewModel.label("ui.settings.close")
                    onClicked: settingsOverlay.visible = false
                }

                GameButton {
                    width: parent.width
                    fillWidth: true
                    text: gameViewModel.label("ui.settings.quit")
                    primary: false
                    outline: true
                    onClicked: Qt.quit()
                }
            }
        }
    }

    Rectangle {
        id: adminEditorOverlay
        anchors.fill: parent
        visible: false
        z: 400

        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        AdminEditorScreen {
            anchors.fill: parent
            onCloseRequested: adminEditorOverlay.visible = false
        }
    }
}
