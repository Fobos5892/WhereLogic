import QtQuick
import QtQuick.Layouts
import ".."
import "../components"

Item {
    id: root
    anchors.fill: parent
    clip: true
    readonly property bool hasDraftAnswer: gameViewModel.userAnswer.trim().length > 0

    readonly property var layoutMap: ({
        "STANDARD": "qrc:/qml/components/layouts/StandardGridLayout.qml",
        "EQUATION": "qrc:/qml/components/layouts/EquationLayout.qml",
        "FULL_MASK": "qrc:/qml/components/layouts/FullMaskLayout.qml",
        "SINGLE_HYBRID": "qrc:/qml/components/layouts/SingleHybridLayout.qml",
        "CHRONOLOGY": "qrc:/qml/components/layouts/ChronologyLayout.qml",
        "TEXT_ONLY": "qrc:/qml/components/layouts/TextOnlyLayout.qml",
        "BLITZ_STANDARD": "qrc:/qml/components/layouts/BlitzGridLayout.qml"
    })

    readonly property bool roundEnded: gameViewModel.currentStage === "STAGE_ROUND_ENDED"

    NeonPanel {
        id: headerPanel
        z: 1
        visible: !root.roundEnded
        anchors.top: parent.top
        anchors.topMargin: Theme.spacing * 0.75
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.margin
        anchors.rightMargin: Theme.margin

        ColumnLayout {
            width: headerPanel.innerWidth
            spacing: Theme.spacing * 0.35

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing * 0.6

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacing * 0.2

                    Text {
                        Layout.fillWidth: true
                        text: gameViewModel.roundTitle
                        color: Theme.gold
                        font.pixelSize: Theme.fontSizeTitle
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: gameViewModel.ruleText
                        color: Qt.lighter(Theme.textSecondary, 1.2)
                        font.pixelSize: Theme.fontSizeBody
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        visible: gameViewModel.ruleText.length > 0
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: gameViewModel.gracePeriodActive
                        text: gameViewModel.label("ui.game.grace_hint")
                        color: Theme.gold
                        font.pixelSize: Theme.fontSizeCaption
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                }

                SmartTimer {
                    id: timer
                    Layout.alignment: Qt.AlignTop | Qt.AlignRight
                }
            }
        }
    }

    Loader {
        id: layoutLoader
        visible: !root.roundEnded
        anchors.top: headerPanel.bottom
        anchors.topMargin: Theme.margin * 1.5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: controlsPanel.visible ? controlsPanel.top : parent.bottom
        anchors.bottomMargin: controlsPanel.visible ? Theme.spacing : 0
        anchors.leftMargin: Theme.margin
        anchors.rightMargin: Theme.margin
        clip: true
        z: 0
        source: layoutMap[gameViewModel.layoutType]
                || "qrc:/qml/components/layouts/StandardGridLayout.qml"

        onLoaded: {
            if (!gameViewModel.cardsFaceUp || !item) {
                return
            }
            if (typeof item.scheduleRevealAnimation === "function") {
                item.scheduleRevealAnimation()
            } else if (typeof item.startRevealAnimation === "function") {
                Qt.callLater(item.startRevealAnimation)
            }
        }
    }

    NeonPanel {
        id: controlsPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.margin
        visible: gameViewModel.showLocalControls && !root.roundEnded

        Column {
            width: controlsPanel.innerWidth
            spacing: Theme.spacing

            GameTextField {
                id: answerField
                width: parent.width * 0.66
                anchors.horizontalCenter: parent.horizontalCenter
                fillWidth: true
                prefixText: ""
                placeholderText: ""
                text: gameViewModel.userAnswer
                horizontalAlignment: Text.AlignHCenter
                onTextEdited: function(t) { gameViewModel.userAnswer = t }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.spacing

                GameButton {
                    visible: gameViewModel.currentStage === "STAGE_MAIN_TURN"
                             || gameViewModel.currentStage === "STAGE_STEAL_TURN"
                    enabled: !root.hasDraftAnswer
                    opacity: enabled ? 1 : 0
                    text: gameViewModel.label("ui.game.submit")
                    gold: true
                    onClicked: gameViewModel.submitAnswer(answerField.text)
                }
                GameButton {
                    visible: gameViewModel.currentStage === "STAGE_CLOSED_CARDS"
                    text: gameViewModel.label("ui.game.ready")
                    success: true
                    onClicked: gameViewModel.ready()
                }
                GameButton {
                    visible: gameViewModel.currentStage === "STAGE_MAIN_TURN"
                           || gameViewModel.currentStage === "STAGE_STEAL_TURN"
                    enabled: !root.hasDraftAnswer && !gameViewModel.gracePeriodActive
                    opacity: enabled ? 1 : 0
                    text: gameViewModel.label("ui.game.transfer")
                    primary: false
                    outline: true
                    onClicked: gameViewModel.transferTurn()
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.spacing
                visible: gameViewModel.showLocalControls
                         && (gameViewModel.currentStage === "STAGE_RESOLUTION"
                             || ((gameViewModel.currentStage === "STAGE_MAIN_TURN"
                                  || gameViewModel.currentStage === "STAGE_STEAL_TURN")
                                 && root.hasDraftAnswer))

                GameButton {
                    text: gameViewModel.teamAName
                    onClicked: gameViewModel.resolveTeamA()
                }
                GameButton {
                    text: gameViewModel.teamBName
                    onClicked: gameViewModel.resolveTeamB()
                }
                GameButton {
                    text: root.hasDraftAnswer
                          ? (gameViewModel.label("ui.game.reject_all") + " (без балла)")
                          : gameViewModel.label("ui.game.reject_all")
                    primary: false
                    outline: true
                    onClicked: gameViewModel.rejectAll()
                }
            }

            Text {
                width: parent.width
                visible: gameViewModel.showLocalControls
                         && (gameViewModel.currentStage === "STAGE_MAIN_TURN"
                             || gameViewModel.currentStage === "STAGE_STEAL_TURN")
                         && root.hasDraftAnswer
                text: "Кому засчитать ответ?"
                color: Theme.success
                font.pixelSize: Theme.fontSizeBody
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: gameViewModel.isCorrectFlash
        color: Theme.success
        opacity: 0.2
        z: 10
    }

    Rectangle {
        anchors.fill: parent
        visible: gameViewModel.isWrongFlash
        color: Theme.danger
        opacity: 0.2
        z: 10
    }

    Item {
        id: roundEndedOverlay
        anchors.fill: parent
        visible: root.roundEnded
        z: 20

        MouseArea {
            anchors.fill: parent
            onClicked: gameViewModel.confirmRoundEnd()
        }

        Text {
            anchors.centerIn: parent
            text: gameViewModel.currentRoundNumber
            color: Theme.gold
            font.pixelSize: Theme.fontSizeHero * 2.5
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
