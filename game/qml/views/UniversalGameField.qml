import QtQuick
import QtQuick.Layouts
import ".."
import "../components"

Item {
    readonly property var layoutMap: ({
        "STANDARD": "qrc:/qml/components/layouts/StandardGridLayout.qml",
        "EQUATION": "qrc:/qml/components/layouts/EquationLayout.qml",
        "FULL_MASK": "qrc:/qml/components/layouts/FullMaskLayout.qml",
        "SINGLE_HYBRID": "qrc:/qml/components/layouts/SingleHybridLayout.qml",
        "CHRONOLOGY": "qrc:/qml/components/layouts/ChronologyLayout.qml",
        "TEXT_ONLY": "qrc:/qml/components/layouts/TextOnlyLayout.qml",
        "BLITZ_STANDARD": "qrc:/qml/components/layouts/BlitzGridLayout.qml"
    })

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 80
        spacing: Theme.spacing

        NeonPanel {
            Layout.fillWidth: true
            implicitHeight: headerCol.implicitHeight + Theme.spacing * 2

            ColumnLayout {
                id: headerCol
                anchors.fill: parent
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        Layout.fillWidth: true
                        text: gameViewModel.roundTitle
                        color: Theme.gold
                        font.pixelSize: Theme.fontSizeTitle
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }

                    SmartTimer { Layout.alignment: Qt.AlignRight | Qt.AlignVCenter }
                }

                Text {
                    Layout.fillWidth: true
                    text: gameViewModel.ruleText
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
                    wrapMode: Text.WordWrap
                    visible: gameViewModel.ruleText.length > 0
                }

                Text {
                    Layout.fillWidth: true
                    text: gameViewModel.hintText.length > 0
                          ? gameViewModel.label("ui.game.hint_format").arg(gameViewModel.hintText)
                          : ""
                    color: Theme.primary
                    font.pixelSize: Theme.fontSizeCaption
                    font.italic: true
                    wrapMode: Text.WordWrap
                    visible: gameViewModel.hintText.length > 0
                              && gameViewModel.cardsFaceUp
                }
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: layoutMap[gameViewModel.layoutType]
                    || "qrc:/qml/components/layouts/StandardGridLayout.qml"
        }

        // Панель ответа — крупные кнопки для пальцев
        NeonPanel {
            Layout.fillWidth: true
            visible: gameViewModel.showLocalControls
            implicitHeight: controlsCol.implicitHeight + Theme.spacing * 2

            ColumnLayout {
                id: controlsCol
                anchors.fill: parent
                spacing: Theme.spacing

                GameTextField {
                    id: answerField
                    Layout.fillWidth: true
                    placeholderText: gameViewModel.label("ui.game.answer_placeholder")
                    text: gameViewModel.userAnswer
                    onTextEdited: function(t) { gameViewModel.userAnswer = t }
                }

                Flow {
                    Layout.fillWidth: true
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
                    Layout.fillWidth: true
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
