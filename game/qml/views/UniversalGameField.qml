import QtQuick
import ".."
import "../components"

Item {
    anchors.fill: parent

    readonly property var layoutMap: ({
        "STANDARD": "qrc:/qml/components/layouts/StandardGridLayout.qml",
        "EQUATION": "qrc:/qml/components/layouts/EquationLayout.qml",
        "FULL_MASK": "qrc:/qml/components/layouts/FullMaskLayout.qml",
        "SINGLE_HYBRID": "qrc:/qml/components/layouts/SingleHybridLayout.qml",
        "CHRONOLOGY": "qrc:/qml/components/layouts/ChronologyLayout.qml",
        "TEXT_ONLY": "qrc:/qml/components/layouts/TextOnlyLayout.qml",
        "BLITZ_STANDARD": "qrc:/qml/components/layouts/BlitzGridLayout.qml"
    })

    NeonPanel {
        id: headerPanel
        anchors.top: parent.top
        anchors.topMargin: 80
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.margin
        anchors.rightMargin: Theme.margin

        Column {
            width: headerPanel.innerWidth
            spacing: 4

            Row {
                width: parent.width
                spacing: Theme.spacing

                Text {
                    width: Math.max(0, parent.width - timer.width - parent.spacing)
                    text: gameViewModel.roundTitle
                    color: Theme.gold
                    font.pixelSize: Theme.fontSizeTitle
                    font.bold: true
                    wrapMode: Text.WordWrap
                }

                SmartTimer {
                    id: timer
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text {
                width: parent.width
                text: gameViewModel.ruleText
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.WordWrap
                visible: gameViewModel.ruleText.length > 0
            }
        }
    }

    Loader {
        id: layoutLoader
        anchors.top: headerPanel.bottom
        anchors.topMargin: Theme.spacing
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: controlsPanel.visible ? controlsPanel.top : parent.bottom
        anchors.bottomMargin: controlsPanel.visible ? Theme.spacing : 0
        anchors.leftMargin: Theme.margin
        anchors.rightMargin: Theme.margin
        source: layoutMap[gameViewModel.layoutType]
                || "qrc:/qml/components/layouts/StandardGridLayout.qml"
    }

    NeonPanel {
        id: controlsPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.margin
        visible: gameViewModel.showLocalControls

        Column {
            width: controlsPanel.innerWidth
            spacing: Theme.spacing

            GameTextField {
                id: answerField
                width: parent.width
                fillWidth: true
                placeholderText: gameViewModel.label("ui.game.answer_placeholder")
                text: gameViewModel.userAnswer
                onTextEdited: function(t) { gameViewModel.userAnswer = t }
            }

            Flow {
                width: parent.width
                spacing: Theme.spacing

                GameButton {
                    text: gameViewModel.label("ui.game.submit")
                    gold: true
                    onClicked: gameViewModel.submitAnswer(answerField.text)
                }
                GameButton {
                    visible: gameViewModel.currentStage === "STAGE_CLOSED_CARDS"
                    text: gameViewModel.label("ui.game.ready")
                    onClicked: gameViewModel.ready()
                }
                GameButton {
                    visible: gameViewModel.currentStage === "STAGE_MAIN_TURN"
                           || gameViewModel.currentStage === "STAGE_STEAL_TURN"
                    text: gameViewModel.label("ui.game.transfer")
                    primary: false
                    outline: true
                    onClicked: gameViewModel.transferTurn()
                }
            }

            Flow {
                width: parent.width
                spacing: Theme.spacing
                visible: gameViewModel.showLocalControls
                         && gameViewModel.currentStage === "STAGE_RESOLUTION"

                GameButton {
                    text: gameViewModel.teamAName
                    onClicked: gameViewModel.resolveTeamA()
                }
                GameButton {
                    text: gameViewModel.teamBName
                    onClicked: gameViewModel.resolveTeamB()
                }
                GameButton {
                    text: gameViewModel.label("ui.game.reject_all")
                    primary: false
                    outline: true
                    onClicked: gameViewModel.rejectAll()
                }
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
}
