import QtQuick
import "."
import "components"

Item {
    id: root
    readonly property bool overlaysOpen: settingsOverlay.visible
        || adminEditorOverlay.visible

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

    function closeAllOverlays() {
        settingsOverlay.visible = false
        adminEditorOverlay.visible = false
    }

    readonly property bool scoreHudVisible: gameViewModel.currentStage !== "STAGE_WELCOME"
        && gameViewModel.currentStage !== "STAGE_TEAM_SETUP"
        && !root.overlaysOpen

    Loader {
        id: frontBuffer
        anchors.top: scoreHudVisible ? topChrome.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: screenSource(gameViewModel.currentStage)
    }

    MissingRevealOverlay {
        anchors.fill: parent
        visible: gameViewModel.currentStage === "STAGE_MISSING_REVEAL"
    }

    Item {
        id: topChrome
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Theme.topBarHeight
        visible: true

        ScoreHUD {
            id: scoreHud
            anchors.left: parent.left
            anchors.right: settingsButton.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.chromeSize + Theme.spacing * 2
            anchors.rightMargin: Theme.spacing
            height: scoreHud.implicitHeight
            visible: scoreHudVisible
        }

        IconButton {
            id: settingsButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: Theme.spacing * 0.5
            width: Theme.chromeSize
            height: Theme.chromeSize
            visible: true
            enabled: !root.overlaysOpen
            opacity: enabled ? 1.0 : 0.45
            iconSource: "qrc:/qml/assets/icon-settings.svg"
            outline: true
            onClicked: {
                root.closeAllOverlays()
                settingsOverlay.visible = true
            }
        }
    }

    Rectangle {
        id: settingsOverlay
        anchors.fill: parent
        color: "#CC0B0C10"
        visible: false
        z: 300

        MouseArea {
            anchors.fill: parent
            onClicked: function(mouse) {
                const local = settingsPanel.mapFromItem(this, mouse.x, mouse.y)
                const insidePanel = local.x >= 0 && local.y >= 0
                                  && local.x <= settingsPanel.width
                                  && local.y <= settingsPanel.height
                if (!insidePanel) {
                    settingsOverlay.visible = false
                }
            }
        }

        NeonPanel {
            id: settingsPanel
            anchors.centerIn: parent
            width: parent.width * 0.92
            height: Math.min(parent.height * 0.9, settingsColumn.implicitHeight + Theme.spacing * 2)

            Column {
                id: settingsColumn
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
                    font.letterSpacing: Theme.spacing * 0.5
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
                        root.closeAllOverlays()
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
                    onClicked: root.closeAllOverlays()
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
            onCloseRequested: root.closeAllOverlays()
        }
    }
}
